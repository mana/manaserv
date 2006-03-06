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

#include "recordset.h"

#include <sstream>
#include <stdexcept>

#include "dalexcept.h"

namespace tmwserv
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
{
    mHeaders.clear();
    mRows.clear();
}


/**
 * Check if the RecordSet is empty.
 */
bool
RecordSet::isEmpty(void) const
{
    return (mRows.size() == 0);
}


/**
 * Get the number of rows.
 *
 * @return the number of rows.
 */
unsigned int
RecordSet::rows(void) const
{
    return mRows.size();
}


/**
 * Get the number of columns.
 *
 * @return the number of columns.
 */
unsigned int
RecordSet::cols(void) const
{
    return mHeaders.size();
}


/**
 * Set the column headers.
 */
void
RecordSet::setColumnHeaders(const Row& headers)
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
{
    const unsigned int nCols = mHeaders.size();

    if (nCols == 0) {
        throw RsColumnHeadersNotSet();
    }

    if (row.size() != nCols) {
        std::ostringstream msg;
        msg << "row has " << row.size() << " columns; "
            << "expected: " << nCols << std::ends;

        throw std::invalid_argument(msg.str());
    }

    mRows.push_back(row);
}


/**
 * Operator()
 */
const std::string&
RecordSet::operator()(const unsigned int row,
                      const unsigned int col) const
{
    if (mHeaders.size() == 0) {
        throw std::invalid_argument(
            "nothing to return as the recordset is empty.");
    }

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
{
    if (mHeaders.size() == 0) {
        throw std::invalid_argument(
            "nothing to return as the recordset is empty.");
    }

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
    const unsigned int nCols = mHeaders.size();
    unsigned int i;
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
    using namespace tmwserv::dal;

    // print the field names first.
    if (rhs.mHeaders.size() > 0) {
        out << "|";
        for (Row::const_iterator it = rhs.mHeaders.begin();
             it != rhs.mHeaders.end();
             ++it)
        {
            out << (*it) << "|";
        }
        out << std::endl << std::endl;
    }

    // and then print every line.
    for (RecordSet::Rows::const_iterator it = rhs.mRows.begin();
         it != rhs.mRows.end();
         ++it)
    {
        out << "|";
        for (Row::const_iterator it2 = (*it).begin();
             it2 != (*it).end();
             ++it2)
        {
            out << (*it2) << "|";
        }
        out << std::endl;
    }

    return out;
}


} // namespace dal
} // namespace tmwserv
