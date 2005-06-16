/*
 *  The Mana World Server
 *  Copyright 2004 The Mana World Development Team
 *
 *  This file is part of The Mana World.
 *
 *  The Mana World  is free software; you can redistribute  it and/or modify it
 *  under the terms of the GNU General  Public License as published by the Free
 *  Software Foundation; either version 2 of the License, or any later version.
 *
 *  The Mana  World is  distributed in  the hope  that it  will be  useful, but
 *  WITHOUT ANY WARRANTY; without even  the implied warranty of MERCHANTABILITY
 *  or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 *  more details.
 *
 *  You should  have received a  copy of the  GNU General Public  License along
 *  with The Mana  World; if not, write to the  Free Software Foundation, Inc.,
 *  59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 *  $Id$
 */


#ifndef _TMW_RECORDSET_H_
#define _TMW_RECORDSET_H_


#include <iostream>
#include <vector>
#include <stdexcept>

#include "dalexcept.h"


namespace tmw
{
namespace dal
{


/**
 * A record from the RecordSet.
 */
typedef std::vector<std::string> Row;


/**
 * A RecordSet to store the result of a SQL query.
 *
 * Limitations: the field values are stored and returned as string,
 * no information about the field data types are stored.
 */
class RecordSet
{
    public:
        /**
         * Default constructor.
         */
        RecordSet(void)
            throw();


        /**
         * Destructor.
         */
        ~RecordSet(void)
            throw();


        /**
         * Remove all the Records.
         */
        void
        clear(void)
            throw();


        /**
         * Get the number of rows.
         *
         * @return the number of rows.
         */
        unsigned int
        rows(void) const
            throw();


        /**
         * Get the number of columns.
         *
         * @return the number of columns.
         */
        unsigned int
        cols(void) const
            throw();


        /**
         * Set the column headers.
         *
         * @param headers the column headers.
         *
         * @exception AlreadySetException if the column headers
         *            are already set.
         */
        void
        setColumnHeaders(const Row& headers)
            throw(AlreadySetException);


        /**
         * Add a new row.
         *
         * This method does not check the field data types, only the number
         * of columns is checked.
         *
         * @param row the new row.
         *
         * @exception std::invalid_argument if the number of columns in the
         *            new row is not equal to the number of column headers.
         */
        void
        add(const Row& row)
            throw(std::invalid_argument);


        /**
         * Operator()
         * Get the value of a particular field of a particular row
         * by field index.
         *
         * @param row the row index.
         * @param col the field index.
         *
         * @return the field value.
         *
         * @exception std::out_of_range if row or col are out of range.
         */
        const std::string&
        operator()(const unsigned int row,
                   const unsigned int col) const
            throw(std::out_of_range);


        /**
         * Operator()
         * Get the value of a particular field of a particular row
         * by field name (slower than by field index).
         *
         * @param row the row index.
         * @param name the field name.
         *
         * @return the field value.
         *
         * @exception std::out_of_range if the row index is out of range.
         * @exception std::invalid_argument if the field name is not found.
         */
        const std::string&
        operator()(const unsigned int row,
                   const std::string& name) const
            throw(std::out_of_range,
                  std::invalid_argument);


        /**
         * Operator<<
         * Append the stringified RecordSet to the output stream.
         *
         * @param out the output stream.
         * @param rhs the right hand side.
         *
         * @return the output stream for chaining.
         */
        friend std::ostream&
        operator<<(std::ostream& out, const RecordSet& rhs);


    private:
        /**
         * Copy constructor.
         */
        RecordSet(const RecordSet& rhs)
            throw();


        /**
         * Operator=
         */
        RecordSet&
        operator=(const RecordSet& rhs)
            throw();


        /**
         * A list of field names.
         */
        Row mHeaders;


        /**
         * A list of records.
         */
        typedef std::vector<Row> Rows;
        Rows mRows;
};


} // namespace dal
} // namespace tmw


#endif // _TMW_RECORDSET_H_
