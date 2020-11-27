#include <sqlite3w.h>

#include <cstring>
#include <sqlite3.h>

using namespace hgl;

SQLite3Error::SQLite3Error(int code, const char * msg): code(code)
{
    if (msg == nullptr)
        msg = sqlite3_errstr(code);

    const auto msg_len = std::strlen(msg) + 1;
    this->msg = reinterpret_cast<char*>(::operator new(msg_len));
    std::memcpy(this->msg, msg, msg_len);
}

SQLite3Error::SQLite3Error(const SQLite3 & db): SQLite3Error(
    sqlite3_errcode(reinterpret_cast<sqlite3*>(db.handle)),
    sqlite3_errmsg(reinterpret_cast<sqlite3*>(db.handle)))
{
}

SQLite3Error::SQLite3Error(SQLite3Error && e):
    code(e.code), msg(e.msg)
{
    e.code = 0;
    e.msg = nullptr;
}

SQLite3Error::SQLite3Error(const SQLite3Error & e): code(e.code)
{
    const auto msg_len = std::strlen(e.msg) + 1;
    this->msg = reinterpret_cast<char*>(::operator new(msg_len));
    std::memcpy(this->msg, e.msg, msg_len);
}

SQLite3Error::~SQLite3Error()
{
    if (this->msg != nullptr)
    {
        ::operator delete(this->msg);
        this->msg = nullptr;
    }
}
