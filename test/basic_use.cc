#include <sqlite3w.h>

#include <cstdlib>
#include <iostream>
#include <iomanip>

using namespace hgl;

int main(int argc, char const *argv[])
{
    SQLite3 db;

    db(R"#(CREATE TABLE Arith_LUT (
        X INTEGER NOT NULL,
        Y INTEGER NOT NULL,
        Sum  INTEGER NOT NULL,
        Diff INTEGER NOT NULL,
        Prod INTEGER NOT NULL,
        Quot REAL );)#");

    SQLite3Stmt stmt_ins(db,
        "INSERT INTO Arith_LUT (X,Y,Sum,Diff,Prod,Quot) VALUES (?,?,?,?,?,?)");

    for (int X = 0; X < 24; X++)
    {
        for (int Y = 0; Y < 24; Y++)
        {
            stmt_ins(X, Y, X + Y, X - Y,
                static_cast<double>(X * Y), static_cast<double>(X) / Y);
            stmt_ins.reset();
        }
    }

    SQLite3Stmt stmt_sel(db,
        "SELECT * FROM Arith_LUT WHERE X=? AND Y=?");

    for (int i = 0; i < 32; i++)
    {
        int x = std::rand() % 14 + 10;
        int y = std::rand() % 14 + 10;

        if (stmt_sel(x, y))
        {
            auto res = stmt_sel.begin();
            std::cout
                << x << '+' << y << '=' << res->read<int>(2) << ' ' << '\t'
                << x << '-' << y << '=' << res->read<int>(3) << ' ' << '\t'
                << x << '*' << y << '=' << res->read<int>(4) << ' ' << '\t'
                << x << '/' << y << '=' << res->read<float>(5) << '\n';
        }
    }

    SQLite3Stmt stmt_sel2 = db.makeSelect("Arith_LUT", nullptr, "X=12");
    if (stmt_sel2())
    {
        std::cout << std::setw(6) << 'X' << std::setw(6) << 'Y'
            << std::setw(6) << "Sum" << std::setw(6) << "Diff"
            << std::setw(6) << "Prod" << std::setw(12) << "Qout" << '\n';
        for (auto & row: stmt_sel2)
        {
            std::cout << std::setw(6) << row.read<int>(0) << std::setw(6)
                << row.read<int>(1) << std::setw(6) << row.read<int>(2)
                << std::setw(6) << row.read<int>(3) << std::setw(6)
                << row.read<int>(4) << std::setw(12) << row.read<float>(5) << '\n';
        }
    }

    db.makeDelete("Arith_LUT", "X=0 AND Y=0")();

    return 0;
}
