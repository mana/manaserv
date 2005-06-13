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


#include <sstream>

#include "recordset.h"


namespace tmw
{
namespace dal
{


/**
 * Default constructor.
 */
Record::Record(void)
    throw()
{
    // NOOP
}


/**
 * Destructor.
 */
Record::~Record(void)
    throw()
{
    // NOOP
}


/**
 * Add a new field.
 */
void
Record::addField(const std::string& name,
                 const std::string& value)
    throw()
{
    mFields.insert(Fields::value_type(name, value));
}


/**
 * Add a new field.
 */
void
Record::addField(const std::string& name,
                 const double value)
    throw()
{
    // convert the number into a string.
    std::ostringstream os;
    os << value << std::ends;

    mFields.insert(Fields::value_type(name, os.str()));
}


/**
 * Get the field value.
 */
const std::string&
Record::get(const std::string& name) const
    throw(std::invalid_argument)
{
    return getAsString(name);
}


/**
 * Get the field value as string.
 */
const std::string&
Record::getAsString(const std::string& name) const
    throw(std::invalid_argument)
{
    Fields::const_iterator it = mFields.find(name);

    if (it == mFields.end()) {
        std::ostringstream msg;
        msg << "unknown field name: " << name << std::ends;

        throw std::invalid_argument(msg.str());
    }

    return it->second;
}


/**
 * Get the field value as a number.
 */
const double
Record::getAsNumber(const std::string& name) const
    throw(std::invalid_argument)
{
    std::istringstream is(getAsString(name));
    double doubleValue;

    // convert the string to a number.
    is >> doubleValue;

    return doubleValue;
}


/**
 * Default constructor.
 */
RecordSet::RecordSet(void)
    throw()
{
    // NOOP
}


/**
 * Destructor.
 */
RecordSet::~RecordSet(void)
    throw()
{
    Rows::iterator it = mRows.begin();
    Rows::iterator it_end = mRows.end();

    for(; it != it_end; ++it) {
        delete *it;
    }
}


/**
 * Add a new Record.
 */
void
RecordSet::addRecord(const Record* record)
    throw()
{
    mRows.push_back(record);
}


/**
 * Get all the rows.
 */
const Rows&
RecordSet::getRows(void) const
    throw()
{
    return mRows;
}


/**
 * Get a particular row.
 */
const Record&
RecordSet::getRow(const unsigned int row) const
    throw(std::out_of_range)
{
    if (row >= mRows.size()) {
        std::ostringstream msg;
        msg << "index out of range: " << row << "; "
            << "num. of records: " << mRows.size() << std::ends;

        throw std::out_of_range(msg.str());
    }

    return *(mRows[row]);
}


} // namespace dal
} // namespace tmw
