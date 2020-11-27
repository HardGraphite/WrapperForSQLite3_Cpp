/**
 * @file sqlite3w.h
 * @brief SQLite3 wrapper
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <utility>

#if defined __GNUC__ // GCC
# ifdef _HGL_BUILD_DL
#   define HGL_API __attribute__((visibility("default")))
# else
#   define HGL_API
# endif
#elif defined _MSC_VER // MSVC
# ifdef _HGL_BUILD_DL
#   define HGL_API __declspec(dllexport)
# else
#   define HGL_API __declspec(dllimport)
# endif
#else // unknown compiler
#   define HGL_API
#endif

namespace hgl
{
    class SQLite3;
    class SQLite3Stmt;

    /// SQLite3 error
    class SQLite3Error final: public std::exception
    {
    private:
        int     code;
        char *  msg;

    public:
        SQLite3Error(int code, const char * msg = nullptr);
        SQLite3Error(const SQLite3 & db);
        SQLite3Error(SQLite3Error && e);
        SQLite3Error(const SQLite3Error & e);
        ~SQLite3Error();

        virtual const char * what() const noexcept override
            { return this->msg; }

        int errcode() const noexcept { return code; }
    };

    /// SQLite3 statement
    class HGL_API SQLite3Stmt
    {
    protected:
        void          * handle;   ///< type: sqlite3_stmt*
        const SQLite3 & database; ///< SQLite3 database connection
        bool            occupied; ///< whether occupied by other object (in use)

        template <typename T> void _bind_val(int col, T val);
        bool _step();

        friend class Curoser;

    public:
        struct Cursor;

        /// value type
        enum class Type { Unknown = 0, Null, Integer, Float, Text, Blob };

        /// row data reader
        struct HGL_API RowReader
        {
        protected:
            Cursor * cur;

        public:
            RowReader(Cursor * c): cur(c) { }

            /**
             * @brief check if usable
             */
            operator bool () const noexcept { return cur != nullptr; }

            /**
             * @brief get row size (column number)
             * 
             * @return row size
             */
            std::size_t size() const noexcept;

            /**
             * @brief get value type
             * 
             * @param col 0 based colome
             * @return value type
             */
            Type type(int col) const noexcept;

            /**
             * @brief read value
             * 
             * @tparam T value type (char*,void*,signed int types, float types or size_t(read length))
             * @param col 0 based column
             * @return the value
             */
            template <typename T> const T read(int col);

            std::int64_t readInteger(int col);
            double readFloat(int col);
            const char * readText(int col);
            const void * readBlob(int col);
            std::size_t readLength(int col);
        };

        /// execution-result iterator
        struct HGL_API Cursor
        {
        protected:
            SQLite3Stmt * stmt;
            RowReader     reader;

            Cursor(): stmt(nullptr), reader(nullptr) { }
            Cursor(SQLite3Stmt * s): stmt(s), reader(this) { }
            void release() { stmt->reset(); stmt = nullptr; }

            friend class SQLite3Stmt;
            friend class SQLite3Stmt::RowReader;

        public:
            Cursor(Cursor &&) = delete;
            Cursor(const Cursor &) = delete;
            ~Cursor() { if (*this) release(); }

            /**
             * @brief check if usable
             */
            operator bool () const noexcept { return stmt != nullptr; }

            /**
             * @brief move to next result
             */
            Cursor & operator++() { if (!stmt->_step()) release(); return *this; }

            /**
             * @brief read result
             */
            RowReader & operator*() noexcept { return reader; }
            RowReader * operator->() noexcept { return &reader; }
        };

        /**
         * @brief compile a SQL statement
         * 
         * @param db SQLite3 connection
         * @param stmt the SQL statement to compile
         */
        SQLite3Stmt(const SQLite3 & db, const char * stmt);

        SQLite3Stmt(SQLite3Stmt &&) = delete;
        SQLite3Stmt(const SQLite3Stmt &) = delete;

        ~SQLite3Stmt();

        /**
         * @brief check if usable
         */
        operator bool () const noexcept { return !occupied; }

        /**
         * @brief execute the statement
         * 
         * @tparam Ts value types
         * @param vals values to bind
         * @return has results
         * 
         * @note use begin() and end() or for-range statement to read the results
         */
        template <typename ... Ts> bool operator()(Ts ... vals)
            { int i = 0; (_bind_val(++i, vals), ...); return _step(); }

        Cursor begin() noexcept { return Cursor(this); }
        Cursor end() noexcept { return Cursor(); }

        /**
         * @brief reset statement
         */
        void reset() noexcept;

        void bindInteger(int col, std::int64_t v);
        void bindFloat(int col, double v);
        void bindText(int col, const char * v);
    };

    /// SQLite3 database connection
    class HGL_API SQLite3
    {
    protected:
        mutable void * handle; ///< type: sqlite3*
        mutable char * buffer; ///< string buffer

        friend class SQLite3Error;
        friend class SQLite3Stmt;

    public:
        using exec_callback_type =
            int(*)(void* param, int col_num, char** col_val, char** col_name);

        /**
         * @brief create a temporary in-memory database
         */
        SQLite3(): SQLite3(":memory:") { }

        /**
         * @brief opening a new database connection
         * 
         * @param filename name of the database file
         */
        explicit SQLite3(const char * filename);

        SQLite3(SQLite3 &&) = delete;
        SQLite3(const SQLite3 &) = delete;

        ~SQLite3();

        /**
         * @brief check if the database is opened
         */
        operator bool () const noexcept { return handle != nullptr; }

        /**
         * @brief opening a new database connection
         * 
         * @param filename name of the database file
         */
        void open(const char * filename);

        /**
         * @brief close the database connection
         * @note this function will be called automatically in distructor
         */
        void close() noexcept;

        /**
         * @brief evaluate simple SQL statments
         * 
         * @param stmts SQL statments to evaluate
         */
        void operator()(const char * stmts);

        /**
         * @brief evaluate simple SQL statments
         * 
         * @param stmts SQL statments to evaluate
         * @param cb callback
         * @param cb_param 1st arguemtn for callback function
         */
        void operator()(const char * stmts, exec_callback_type cb, void * cb_param);

        /**
         * @brief get last error message
         * 
         * @return message string
         */
        const char * getErrMsg() noexcept;

        SQLite3Stmt makeInsert(const char * table, const char * names, const char * values);
        SQLite3Stmt makeInsert(const char * table, const char * values);
        SQLite3Stmt makeSelect(const char * table, const char * names = nullptr, const char * where = nullptr);
        SQLite3Stmt makeUpdate(const char * table,
            std::initializer_list<std::pair<const char*, const char*>> name_vals, const char * where);
        SQLite3Stmt makeDelete(const char * table, const char * where);
    };

} // namespace hgl


#include <type_traits>

template <typename T> inline void hgl::SQLite3Stmt::_bind_val(int col, T val)
{
    if constexpr (std::is_integral<T>::value)
        bindInteger(col, val);
    else if constexpr (std::is_floating_point<T>::value)
        bindFloat(col, val);
    else if constexpr (std::is_same<const char*, T>::value)
        bindText(val);
    else
        static_assert(std::is_floating_point<T>::value, "invalid type T");
}

template <typename T> inline const T hgl::SQLite3Stmt::RowReader::read(int col)
{
    using U = typename std::remove_cv<T>::type;

    if constexpr (std::is_same<std::size_t, U>::value)
        return readLength(col);
    else if constexpr (std::is_same<char*, U>::value)
        return readText(col);
    else if constexpr (std::is_same<void*, U>::value)
        return readBlob(col);
    else if constexpr (std::is_integral<U>::value && std::is_signed<U>::value)
        return readInteger(col);
    else if constexpr (std::is_floating_point<U>::value)
        return readFloat(col);
    else
        static_assert(std::is_floating_point<U>::value, "invalid type T");
}
