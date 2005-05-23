/* 
   SQLiteWrapper.cpp

   Copyright (C) 2004 René Nyffenegger

   This source code is provided 'as-is', without any express or implied
   warranty. In no event will the author be held liable for any damages
   arising from the use of this software.

   Permission is granted to anyone to use this software for any purpose,
   including commercial applications, and to alter it and redistribute it
   freely, subject to the following restrictions:

   1. The origin of this source code must not be misrepresented; you must not
      claim that you wrote the original source code. If you use this source code
      in a product, an acknowledgment in the product documentation would be
      appreciated but is not required.

   2. Altered source versions must be plainly marked as such, and must not be
      misrepresented as being the original source code.

   3. This notice may not be removed or altered from any source distribution.

   René Nyffenegger rene.nyffenegger@adp-gmbh.ch
   Modified by Mateusz Kaduk mateusz.kaduk@gmail.com

*/

#include "SQLiteWrapper.h"

SQLiteWrapper::SQLiteWrapper() : db_(0) {
}

bool SQLiteWrapper::Open(std::string const& db_file) {
  if (sqlite3_open(db_file.c_str(), &db_) != SQLITE_OK) {
    return false;
  }
  return true;
} 

bool SQLiteWrapper::Close() {
    if (sqlite3_close(db_) != SQLITE_OK) {
    return false;
  }
  return true;
}

bool SQLiteWrapper::SelectStmt(std::string const& stmt, ResultTable& res) {
  char *errmsg;
  int   ret;

  res.reset();

  ret = sqlite3_exec(db_, stmt.c_str(), SelectCallback, static_cast<void*> (&res), &errmsg);

  if (ret != SQLITE_OK) {
    return false;
  }
  return true;
//  if (ret != SQLITE_OK) {
//    std::cout << stmt << " [" << errmsg << "]" << std::endl;
//  }
}

// TODO parameter p_col_names
int SQLiteWrapper::SelectCallback(void *p_data, int num_fields, char **p_fields, char**  /*p_col_names*/) {
  ResultTable* res = reinterpret_cast<ResultTable*>(p_data);

  ResultRecord record;

  for(int i=0; i < num_fields; i++) {
    record.fields_.push_back(p_fields[i]);
  }

  res->records_.push_back(record);

  return 0;
}

SQLiteStatement* SQLiteWrapper::Statement(std::string const& statement) {
  SQLiteStatement* stmt;
  try {
    stmt = new SQLiteStatement(statement, db_);
    return stmt;
  }
  catch (const char* e) {
    return 0;
  }
}

SQLiteStatement::SQLiteStatement(std::string const& statement, sqlite3* db) {
  if ( sqlite3_prepare(
         db, 
         statement.c_str(),  // stmt
        -1,                  // If than zero, then stmt is read up to the first nul terminator
        &stmt_,
         0                   // Pointer to unused portion of stmt
       )
       != SQLITE_OK) {
    throw sqlite3_errmsg(db);
  }

  if (!stmt_) {
    throw "stmt_ is 0";
  }
}

SQLiteStatement::~SQLiteStatement() {
}

SQLiteStatement::SQLiteStatement() :
  stmt_       (0)
{
}

bool SQLiteStatement::Bind(int pos_zero_indexed, std::string const& value) {
  if (sqlite3_bind_text (
         stmt_,
         pos_zero_indexed+1,  // Index of wildcard
         value.c_str(),
         value.length(),      // length of text
         SQLITE_TRANSIENT     // SQLITE_TRANSIENT: SQLite makes its own copy
         )
       != SQLITE_OK) {
     return false;
  }
  return true;
}

bool SQLiteStatement::Bind(int pos_zero_indexed, double value) {
  if (sqlite3_bind_double(
          stmt_,
          pos_zero_indexed+1,  // Index of wildcard
          value
          )
        != SQLITE_OK) {
      return false;
  }
  return true;
}

bool SQLiteStatement::Bind(int pos_zero_indexed, int value) {
  if (sqlite3_bind_int(
          stmt_,
          pos_zero_indexed+1,  // Index of wildcard
           value 
          )
        != SQLITE_OK) {
      return false;
  }
  return true;
}

bool SQLiteStatement::BindNull(int pos_zero_indexed) {
  if (sqlite3_bind_null(
          stmt_,
          pos_zero_indexed+1  // Index of wildcard
          )
        != SQLITE_OK) {
      return false;
  }
  return true;
}

bool SQLiteStatement::Execute() {
  int rc = sqlite3_step(stmt_);
  if (rc == SQLITE_BUSY) {
    //::MessageBox(0, "SQLITE_BUSY", 0, 0); 
    return false;
  }
  if (rc == SQLITE_ERROR) {
    //::MessageBox(0, "SQLITE_ERROR", 0, 0); 
    return false;
  }
  if (rc == SQLITE_MISUSE) {
    //::MessageBox(0, "SQLITE_ERROR", 0, 0); 
    return false;
  }
  if (rc != SQLITE_DONE) {
    //sqlite3_reset(stmt_);
    return false;
  }
  sqlite3_reset(stmt_);
  return true;
}

SQLiteStatement::dataType SQLiteStatement::DataType(int pos_zero_indexed) {
  return dataType(sqlite3_column_type(stmt_, pos_zero_indexed));
}

int SQLiteStatement::ValueInt(int pos_zero_indexed) {
  return sqlite3_column_int(stmt_, pos_zero_indexed);
}

std::string SQLiteStatement::ValueString(int pos_zero_indexed) {
  return std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt_, pos_zero_indexed)));
}

bool SQLiteStatement::RestartSelect() {
  sqlite3_reset(stmt_);
  return true;
}

bool SQLiteStatement::Reset() {
  int rc = sqlite3_step(stmt_);

  sqlite3_reset(stmt_);

  if (rc == SQLITE_ROW) return true;
  return false;
}

bool SQLiteStatement::NextRow() {
  int rc = sqlite3_step(stmt_);

  if (rc == SQLITE_ROW   ) {
    return true;
  }
  if (rc == SQLITE_DONE  ) {
    sqlite3_reset(stmt_);
    return false;
  } 
  else if (rc == SQLITE_MISUSE) {
    //::MessageBox(0,  "SQLiteStatement::NextRow SQLITE_MISUSE", 0, 0);
  } 
  else if (rc == SQLITE_BUSY  ) {
    //::MessageBox(0, "SQLiteStatement::NextRow SQLITE_BUSY", 0, 0);
  } 
  else if (rc == SQLITE_ERROR ) {
    //::MessageBox(0, "SQLiteStatement::NextRow SQLITE_ERROR", 0, 0);
  }
  return false;
}

bool SQLiteWrapper::DirectStatement(std::string const& stmt) {
  char *errmsg;
  int   ret;

  ret = sqlite3_exec(db_, stmt.c_str(), 0, 0, &errmsg);

  if(ret != SQLITE_OK) {
    return false;
  }
  return true;

  //if(ret != SQLITE_OK) {
  //  std::cout << stmt << " [" << errmsg << "]" << std::endl;
  //}
}

std::string SQLiteWrapper::LastError() {
  return sqlite3_errmsg(db_);
}

bool SQLiteWrapper::Begin() {
  return DirectStatement("begin");
}

bool SQLiteWrapper::Commit() {
  return DirectStatement("commit");
}

bool SQLiteWrapper::Rollback() {
  return DirectStatement("rollback");
}
