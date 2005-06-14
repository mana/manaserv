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
    // NOOP
}


/**
 * Remove all the Records.
 */
void
RecordSet::clear(void)
    throw()
{
    mHeaders.clear();
    mRows.clear();
}


/**
 * Get the number of rows.
 *
 * @return the number of rows.
 */
unsigned int
RecordSet::rows(void)
    throw()
{
    return mRows.size();
}


/**
 * Get the number of columns.
 *
 * @return the number of columns.
 */
unsigned int
RecordSet::cols(void)
    throw()
{
    return mHeaders.size();
}


/**
 * Set the column headers.
 */
void
RecordSet::setColumnHeaders(const Row& headers)
    throw(AlreadySetException)
{
    if (mHeaders.size() > 0) {
        throw AlreadySetException();
    }

    mHeaders = headers;
}


/**
 * Add a new row.
 */
void
RecordSet::add(const Row& row)
    throw(std::invalid_argument)
{
    if (row.size() != mHeaders.size()) {
        throw std::invalid_argument(
            "the new row does not have the required number of columns.");
    }

    mRows.push_back(row);
}


/**
 * Operator()
 */
const std::string&
RecordSet::operator()(const unsigned int row,
                      const unsigned int col) const
    throw(std::out_of_range)
{
    if ((row >= mRows.size()) || (col >= mHeaders.size())) {
        std::ostringstream os;
        os << "(" << row << ", " << col << ") is out of range; "
           << "max rows: " << mRows.size()
           << ", max cols: " << mHeaders.size() << std::ends;

        throw std::out_of_range(os.str());
    }

    return mRows[row][col];
}


/**
 * Operator()
 */
const std::string&
RecordSet::operator()(const unsigned int row,
                      const std::string& name) const
    throw(std::out_of_range,
          std::invalid_argument)
{
    if (row >= mRows.size()) {
        std::ostringstream os;
        os << "row " << row << " is out of range; "
           << "max rows: " << mRows.size() << std::ends;

        throw std::out_of_range(os.str());
    }

    Row::const_iterator it = std::find(mHeaders.begin(),
                                       mHeaders.end(),
                                       name);
    if (it == mHeaders.end()) {
        std::ostringstream os;
        os << "field " << name << " does not exist." << std::ends;

        throw std::invalid_argument(os.str());
    }

    // find the field index.
    const int nCols = mHeaders.size();
    int i;
    for (i = 0; i < nCols; ++i) {
        if (mHeaders[i] == name) {
            break;
        }
    }

    return mRows[row][i];
}


/**
 * Operator<<
 */
std::ostream&
operator<<(std::ostream& out, const RecordSet& rhs)
{
    using namespace tmw::dal;

    // print the field names first.
    if (rhs.mHeaders.size() > 0) {
        Row::const_iterator it = rhs.mHeaders.begin();
        out << "[ " << (*it);
        it++;
        for (; it != rhs.mHeaders.end(); ++it) {
            out << " | " << (*it);
        }
        out << " ]" << std::endl;
    }

    // and then print every line.
    for (RecordSet::Rows::const_iterator it = rhs.mRows.begin();
         it != rhs.mRows.end();
         ++it)
    {
        Row::const_iterator it2 = (*it).begin();
        out << "[ " << (*it2);
        it2++;
        for (; it2 != (*it).end(); ++it2) {
            out << " | " << (*it2);
        }
        out << " ]" << std::endl;
    }

    return out;
}


} // namespace dal
} // namespace tmw
