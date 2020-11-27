#include <sqlite3w.h>

#include <cstring>
#include <sqlite3.h>

using namespace hgl;

static constexpr auto bufsize = 1024;

SQLite3::SQLite3(const char * filename):
    handle(nullptr), buffer(reinterpret_cast<char*>(::operator new(bufsize)))
{
    if (filename != nullptr)
        this->open(filename);
}

SQLite3::~SQLite3()
{
    this->close();
    ::operator delete(this->buffer);
}

void SQLite3::open(const char * filename)
{
    if (this->handle != nullptr)
        this->close();

    if (sqlite3_open(filename, reinterpret_cast<sqlite3**>(&this->handle)) != SQLITE_OK)
    {
        this->handle = nullptr;
        throw SQLite3Error(*this);
    }
}

void SQLite3::close() noexcept
{
    if (this->handle == nullptr)
        return;

    sqlite3_close(reinterpret_cast<sqlite3*>(this->handle));
    this->handle = nullptr;
}

void SQLite3::operator()(const char * stmts)
{
    char * errmsg;
    const auto ret = sqlite3_exec(reinterpret_cast<sqlite3*>(this->handle),
        stmts, nullptr, nullptr, &errmsg);

    if (ret != SQLITE_OK)
    {
        SQLite3Error error(ret, errmsg);
        sqlite3_free(errmsg);
        throw error;
    }
}

void SQLite3::operator()(const char * stmts, exec_callback_type cb, void * cb_param)
{
    char * errmsg;
    const auto ret = sqlite3_exec(reinterpret_cast<sqlite3*>(this->handle),
        stmts, cb, cb_param, &errmsg);

    if (ret != SQLITE_OK)
    {
        SQLite3Error error(ret, errmsg);
        sqlite3_free(errmsg);
        throw error;
    }
}

const char * SQLite3::getErrMsg() noexcept
{
    const auto msg = sqlite3_errmsg(reinterpret_cast<sqlite3*>(this->handle));
    return msg;
}

static const char _text_delete_from[] = "DELETE FROM ";
static const char _text_insert_into[] = "INSERT INTO ";
static const char _text_select[] = "SELECT ";
static const char _text_update[] = "UPDATE ";

static const char _text_from[] = " FROM ";
static const char _text_set[] = " SET ";
static const char _text_values[] = " VALUES ";
static const char _text_where[] = " WHERE ";

#define PUT_PTEXT(PTR, TEXT) \
    std::memcpy(p, _text_ ##TEXT, sizeof(_text_ ##TEXT) - 1); \
    p += (sizeof(_text_ ##TEXT) - 1); \
// PUT_PTEXT

#define PUT_CSTR(PTR, STR) { \
    auto const _len = std::strlen(STR); \
    std::memcpy(PTR, STR, _len); \
    PTR += _len; } \
// PUT_CSTR

#define PUT_CHAR(PTR, CH) *(PTR++) = (CH)

#define PUT_END(PTR) { PUT_CHAR(PTR, ';'); *PTR = '\0'; }

SQLite3Stmt SQLite3::makeInsert(
    const char * table, const char * names, const char * values)
{
    char * p = this->buffer;

    PUT_PTEXT(p, insert_into); PUT_CSTR(p, table);

    PUT_CHAR(p, '('); PUT_CSTR(p, names); PUT_CHAR(p, ')');

    PUT_PTEXT(p, values); PUT_CHAR(p, '('); PUT_CSTR(p, values); PUT_CHAR(p, ')');

    PUT_END(p);

    return SQLite3Stmt(*this, this->buffer);
}

SQLite3Stmt SQLite3::makeInsert(const char * table, const char * values)
{
    char * p = this->buffer;

    PUT_PTEXT(p, insert_into); PUT_CSTR(p, table);

    PUT_PTEXT(p, values); PUT_CHAR(p, '('); PUT_CSTR(p, values); PUT_CHAR(p, ')');

    PUT_END(p);

    return SQLite3Stmt(*this, this->buffer);
}

SQLite3Stmt SQLite3::makeSelect(
    const char * table, const char * names, const char * where)
{
    char * p = this->buffer;

    PUT_PTEXT(p, select);

    if (names == nullptr)
    {
        PUT_CHAR(p, '*');
    }
    else
    {
        PUT_CSTR(p, names);
    }

    PUT_PTEXT(p, from); PUT_CSTR(p, table);

    if (where != nullptr)
    {
        PUT_PTEXT(p, where); PUT_CSTR(p, where);
    }

    PUT_END(p);

    return SQLite3Stmt(*this, this->buffer);
}

SQLite3Stmt SQLite3::makeUpdate(const char * table,
    std::initializer_list<std::pair<const char*, const char*>> name_vals, const char * where)
{

    char * p = this->buffer;

    PUT_PTEXT(p, update); PUT_CSTR(p, table);

    PUT_PTEXT(p, set);

    for (auto & [name, val]: name_vals)
    {
        PUT_CSTR(p, name); PUT_CHAR(p, '='); PUT_CSTR(p, val);
    }

    PUT_PTEXT(p, where); PUT_CSTR(p, where);

    PUT_END(p);

    return SQLite3Stmt(*this, this->buffer);
}

SQLite3Stmt SQLite3::makeDelete(const char * table, const char * where)
{
    char * p = this->buffer;

    PUT_PTEXT(p, delete_from); PUT_CSTR(p, table);

    PUT_PTEXT(p, where); PUT_CSTR(p, where);

    PUT_END(p);

    return SQLite3Stmt(*this, this->buffer);
}
