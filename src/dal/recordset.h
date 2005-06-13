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


#include <map>
#include <vector>
#include <stdexcept>


namespace tmw
{
namespace dal
{


/**
 * A Record.
 */
class Record
{
    public:
        /**
         * Default constructor.
         */
        Record(void)
            throw();


        /**
         * Destructor.
         */
        ~Record(void)
            throw();


        /**
         * Add a new field.
         *
         * @param name the field name.
         * @param value the field value
         */
        void
        addField(const std::string& name,
                 const std::string& value)
            throw();


        /**
         * Add a new field.
         *
         * @param name the field name.
         * @param value the field value
         */
        void
        addField(const std::string& name,
                 const double value)
            throw();


        /**
         * Get the field value.
         *
         * This method is an alias of getAsString().
         *
         * @param name the field name.
         *
         * @return the value as string.
         *
         * @exception std::invalid_argument if the field is not found.
         */
        const std::string&
        get(const std::string& name) const
            throw(std::invalid_argument);


        /**
         * Get the field value as string.
         *
         * @param name the field name.
         *
         * @return the value as string.
         *
         * @exception std::invalid_argument if the field is not found.
         */
        const std::string&
        getAsString(const std::string& name) const
            throw(std::invalid_argument);


        /**
         * Get the field value as a number.
         *
         * @param name the field name.
         *
         * @return the value as float.
         *
         * @exception std::invalid_argument if the field is not found.
         */
        const double
        getAsNumber(const std::string& name) const
            throw(std::invalid_argument);


    private:
        /**
         * A map to hold the field names and their associated values.
         */
        typedef std::map<std::string, std::string> Fields;
        Fields mFields;
};


/**
 * A list of Records.
 */
typedef std::vector<const Record*> Rows;


/**
 * A RecordSet to store the result of a SQL query.
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
         * Add a new Record.
         *
         * @param record the new record.
         */
        void
        addRecord(const Record*)
            throw();


        /**
         * Get all the rows.
         *
         * @return all the rows of the RecordSet.
         */
        const Rows&
        getRows(void) const
            throw();


        /**
         * Get a particular row.
         *
         * @param row the row index.
         *
         * @return the Record at the specified row index.
         *
         * @exception std::out_of_range
         */
        const Record&
        getRow(const unsigned int row) const
            throw(std::out_of_range);


    private:
        /**
         * A list of Records.
         */
        Rows mRows;
};


} // namespace dal
} // namespace tmw


#endif // _TMW_RECORDSET_H_
