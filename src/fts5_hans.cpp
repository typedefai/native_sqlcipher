#include "fts5_hans.h"
#include <stdio.h>
#include <mutex>
#include <string>
#include <vector>
#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#include <io.h>
#define access _access
#define F_OK 0
#define getcwd _getcwd
#define PATH_MAX MAX_PATH
#else
#include <unistd.h>
#include <limits.h>
#endif
#include "fts5.h"

#include "cppjieba/Jieba.hpp"
#include "cppjieba/KeywordExtractor.hpp"

SQLITE_EXTENSION_INIT1

static std::mutex g_jieba_mutex;
static cppjieba::Jieba *g_jieba = nullptr;
static std::vector<std::string> g_dicts;

static bool file_exists(const std::string &path)
{
    return access(path.c_str(), F_OK) == 0;
}

static int set_jieba_dicts(const char *root_path)
{

    static const char *default_files[5] = {
        "jieba.dict.utf8", "hmm_model.utf8", "user.dict.utf8", "idf.utf8", "stop_words.utf8"};
    std::vector<std::string> dict_paths;
    std::string root = root_path ? root_path : "";
    if (!root.empty() && root.back() != '/' && root.back() != '\\')
        root += '/';

    for (int i = 0; i < 5; ++i)
    {
        std::string path = root + default_files[i];
        if (!file_exists(path))
        {
            fprintf(stderr, "Dictionary file does not exist: %s\n", path.c_str());
            return -1; // Or your error code
        }
        dict_paths.push_back(path);
    }
    {
        std::lock_guard<std::mutex> lock(g_jieba_mutex);
        g_dicts = dict_paths;

        // Reset the Jieba instance so it will be recreated with new dicts
        if (g_jieba)
        {
            delete g_jieba;
            g_jieba = nullptr;
        }
    }
    return 0;
}

static cppjieba::Jieba *get_jieba_instance()
{
    std::lock_guard<std::mutex> lock(g_jieba_mutex);
    if (!g_jieba)
    {
        if (g_dicts.size() < 5)
            return nullptr;
        try
        {
            g_jieba = new cppjieba::Jieba(
                g_dicts[0], g_dicts[1], g_dicts[2], g_dicts[3], g_dicts[4]);
        }
        catch (const std::exception &e)
        {
            fprintf(stderr, "jieba initialization failed: %s\n", e.what());
            g_jieba = nullptr;
        }
    }
    return g_jieba;
}

enum class TokenCategory
{
    CJK,
    ALPHA,
    NUMBER,
    OTHER
};

static TokenCategory from_char(char c)
{
    if ((c & 0x80) != 0)
        return TokenCategory::CJK;
    else if (isalpha(c))
        return TokenCategory::ALPHA;
    else if (isdigit(c))
        return TokenCategory::NUMBER;
    else
        return TokenCategory::OTHER;
}

typedef int (*xTokenFn)(void *, int, const char *, int, int, int);

int fts5_hans_xCreate(void *sqlite3, const char **azArg, int nArg, Fts5Tokenizer **ppOut)
{
    Fts5HansTokenizer *t = (Fts5HansTokenizer *)sqlite3_malloc(sizeof(Fts5HansTokenizer));
    if (!t)
        return SQLITE_NOMEM;
    t->use_hmm = true;
    for (int i = 0; i < nArg; i++)
    {
        if (strcmp(azArg[i], "no_hmm") == 0)
            t->use_hmm = false;
    }
    if (!get_jieba_instance())
    {
        sqlite3_free(t);
        return SQLITE_ERROR;
    }
    *ppOut = reinterpret_cast<Fts5Tokenizer *>(t);
    return SQLITE_OK;
}

int fts5_hans_xTokenize(Fts5Tokenizer *pTokenizer, void *pCtx, int flags, const char *pText, int nText, xTokenFn xToken)
{
    Fts5HansTokenizer *p = (Fts5HansTokenizer *)pTokenizer;
    if (nText <= 0 || !pText)
        return SQLITE_OK;
    char *text_copy = (char *)sqlite3_malloc(nText + 1);
    if (!text_copy)
        return SQLITE_NOMEM;
    memcpy(text_copy, pText, nText);
    text_copy[nText] = '\0';
    std::string text(text_copy);
    std::vector<cppjieba::Word> words;
    cppjieba::Jieba *jieba = get_jieba_instance();
    if (!jieba)
    {
        sqlite3_free(text_copy);
        return SQLITE_ERROR;
    }
    jieba->Cut(text_copy, words, p->use_hmm);
    for (const auto &word : words)
    {
        if (word.word.empty())
            continue;
        TokenCategory category = from_char(word.word[0]);
        for (auto c : word.word)
        {
            if (from_char(c) != category)
            {
                category = TokenCategory::OTHER;
                break;
            }
        }
        int rc = xToken(pCtx, 0, pText + word.offset, word.word.length(), word.offset, word.offset + word.word.length());
        if (rc != SQLITE_OK)
        {
            sqlite3_free(text_copy);
            return rc;
        }
    }
    sqlite3_free(text_copy);
    return SQLITE_OK;
}

void fts5_hans_xDelete(Fts5Tokenizer *p)
{
    Fts5HansTokenizer *t = (Fts5HansTokenizer *)p;
    sqlite3_free(t);
}

static fts5_tokenizer tokenizer = {
    fts5_hans_xCreate,
    fts5_hans_xDelete,
    fts5_hans_xTokenize,
};

static int fts5_api_from_db(sqlite3 *db, fts5_api **ppApi)
{
    sqlite3_stmt *pStmt = 0;
    *ppApi = 0;
    int rc = sqlite3_prepare_v2(db, "SELECT fts5(?1)", -1, &pStmt, 0);
    if (rc != SQLITE_OK)
        return rc;
    sqlite3_bind_pointer(pStmt, 1, (void *)ppApi, "fts5_api_ptr", NULL);
    sqlite3_step(pStmt);
    sqlite3_finalize(pStmt);
    return SQLITE_OK;
}

int fts5_hans_tokenizer_register(sqlite3 *db)
{
    int rc = SQLITE_OK;
    fts5_api *fts5api = 0;
    rc = fts5_api_from_db(db, &fts5api);
    if (rc != SQLITE_OK || !fts5api)
        return (rc != SQLITE_OK) ? rc : SQLITE_ERROR;
    rc = fts5api->xCreateTokenizer(fts5api, "jieba", reinterpret_cast<void *>(fts5api), &tokenizer, NULL);
    return rc;
}

static void fts5_hans_print_dict_paths_sql(sqlite3_context *ctx, int argc, sqlite3_value **argv)
{
    std::string msg = "Current jieba dictionary paths:\n";
    if (g_dicts.empty())
    {
        msg += "  (not set)\n";
    }
    else
    {
        for (size_t i = 0; i < g_dicts.size(); ++i)
        {
            msg += "  ";
            msg += g_dicts[i];
            msg += "\n";
        }
    }
    sqlite3_result_text(ctx, msg.c_str(), -1, SQLITE_TRANSIENT);
}

static void enable_jieba_tokenizer(sqlite3_context *ctx, int argc, sqlite3_value **argv)
{
    const char *root = nullptr;
    char cwd[PATH_MAX] = {0};

    if (argc != 1)
    {
        sqlite3_result_error(ctx, "usage: enable_jieba('/path/to/dir')", -1);
        return;
    }

    if (sqlite3_value_type(argv[0]) == SQLITE_NULL ||
        (sqlite3_value_type(argv[0]) == SQLITE_TEXT && sqlite3_value_bytes(argv[0]) == 0))
    {
        // Use current working directory if parameter is empty or NULL
        if (!getcwd(cwd, sizeof(cwd)))
        {
            sqlite3_result_error(ctx, "getcwd failed", -1);
            return;
        }
        root = cwd;
    }
    else if (sqlite3_value_type(argv[0]) == SQLITE_TEXT)
    {
        root = (const char *)sqlite3_value_text(argv[0]);
    }
    else
    {
        sqlite3_result_error(ctx, "usage: enable_jieba('/path/to/dir')", -1);
        return;
    }

    int rc = set_jieba_dicts(root);
    if (rc != 0)
    {
        sqlite3_result_error(ctx, "failed to load jieba dicts", -1);
        return;
    }
    sqlite3 *db = sqlite3_context_db_handle(ctx);
    rc = fts5_hans_tokenizer_register(db);
    if (rc != 0)
    {
        sqlite3_result_error(ctx, "register tokenizer failed", -1);
        return;
    }
    sqlite3_result_text(ctx, "ok", -1, SQLITE_STATIC);
}

int sqlite3_fts5_hans_init(sqlite3 *db, char **pzErrMsg, const sqlite3_api_routines *pApi)
{
    SQLITE_EXTENSION_INIT2(pApi);
    int rc = sqlite3_create_function(db, "enable_jieba", 1, SQLITE_UTF8, NULL, enable_jieba_tokenizer, NULL, NULL);
    if (rc != SQLITE_OK)
        return rc;
    return sqlite3_create_function(db, "print_jieba_dict_paths", 0, SQLITE_UTF8, NULL, fts5_hans_print_dict_paths_sql, NULL, NULL);
}
