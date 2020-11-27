#include <sqlite3w.h>

#include <chrono>
#include <cstring>
#include <thread>
#include <sqlite3.h>

using namespace hgl;

SQLite3Stmt::SQLite3Stmt(const SQLite3 & db, const char * stmt):
    database(db), occupied(false)
{
    const auto ret = sqlite3_prepare_v2(
        reinterpret_cast<sqlite3*>(db.handle), stmt, -1,
        reinterpret_cast<sqlite3_stmt**>(&this->handle), nullptr);
    if (ret != SQLITE_OK)
    {
        this->handle = nullptr;
        throw SQLite3Error(this->database);
    }
}

SQLite3Stmt::~SQLite3Stmt()
{
    if (this->handle == nullptr)
        return;

    sqlite3_finalize(reinterpret_cast<sqlite3_stmt*>(this->handle));
    this->handle = nullptr;
}

void SQLite3Stmt::bindInteger(int col, std::int64_t v)
{
    auto stmt = reinterpret_cast<sqlite3_stmt*>(this->handle);
    auto const res = sqlite3_bind_int64(stmt, col, v);
    if (res != SQLITE_OK) throw SQLite3Error(this->database);
}

void SQLite3Stmt::bindFloat(int col, double v)
{
    auto stmt = reinterpret_cast<sqlite3_stmt*>(this->handle);
    auto const res = sqlite3_bind_double(stmt, col, v);
    if (res != SQLITE_OK) throw SQLite3Error(this->database);
}

void SQLite3Stmt::bindText(int col, const char * v)
{
    auto stmt = reinterpret_cast<sqlite3_stmt*>(this->handle);
    auto const res = sqlite3_bind_text(stmt, col, v, std::strlen(v), SQLITE_STATIC);
    if (res != SQLITE_OK) throw SQLite3Error(this->database);
}


void SQLite3Stmt::reset() noexcept
{
    this->occupied = false;
    sqlite3_reset(reinterpret_cast<sqlite3_stmt*>(this->handle));
}


bool SQLite3Stmt::_step()
{
    if (!*this)
        return false;

    int retry_cnt = 0, res;

_L_START:
    res = sqlite3_step(reinterpret_cast<sqlite3_stmt*>(this->handle));

    switch (res)
    {
    case SQLITE_BUSY:
        if (++retry_cnt < 16)
        {
            using std::this_thread::sleep_for;
            using std::chrono::milliseconds;
            sleep_for(milliseconds(250));
            goto _L_START;
        }
        throw SQLite3Error(this->database);

    case SQLITE_ROW:
        return true;

    case SQLITE_DONE:
        return false;

    default:
        throw SQLite3Error(this->database);
    }
}


std::size_t SQLite3Stmt::RowReader::size() const noexcept
{
    return (*this) ?
        sqlite3_column_count(reinterpret_cast<sqlite3_stmt*>(this->cur->stmt->handle)):
        0;
}

SQLite3Stmt::Type SQLite3Stmt::RowReader::type(int col) const noexcept
{
    if (!*this)
        return Type::Unknown;

    switch (sqlite3_column_type(
        reinterpret_cast<sqlite3_stmt*>(this->cur->stmt->handle), col))
    {
        case SQLITE_NULL: return Type::Null;
        case SQLITE_INTEGER: return Type::Integer;
        case SQLITE_FLOAT: return Type::Float;
        case SQLITE_TEXT: return Type::Text;
        case SQLITE_BLOB: return Type::Blob;
        default: return Type::Unknown;
    }
}

std::int64_t SQLite3Stmt::RowReader::readInteger(int col)
{
    auto stmt = reinterpret_cast<sqlite3_stmt*>(this->cur->stmt->handle);
    return sqlite3_column_int64(stmt, col);
}

double SQLite3Stmt::RowReader::readFloat(int col)
{
    auto stmt = reinterpret_cast<sqlite3_stmt*>(this->cur->stmt->handle);
    return sqlite3_column_double(stmt, col);
}

const char * SQLite3Stmt::RowReader::readText(int col)
{
    auto stmt = reinterpret_cast<sqlite3_stmt*>(this->cur->stmt->handle);
    auto const val = sqlite3_column_text(stmt, col);
    return (val == nullptr) ? "" : reinterpret_cast<const char *>(val);
}

const void * SQLite3Stmt::RowReader::readBlob(int col)
{
    auto stmt = reinterpret_cast<sqlite3_stmt*>(this->cur->stmt->handle);
    auto const val = sqlite3_column_blob(stmt, col);
    return (val == nullptr) ? "" : val;
}

std::size_t SQLite3Stmt::RowReader::readLength(int col)
{
    auto stmt = reinterpret_cast<sqlite3_stmt*>(this->cur->stmt->handle);
    return sqlite3_column_bytes(stmt, col);
}
