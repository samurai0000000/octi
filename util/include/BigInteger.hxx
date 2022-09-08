/*************************************************************************
 * $Id: BigInteger.hxx,v 1.1 2000/11/06 22:30:46 samurai Exp $
 * $Author: samurai $
 * $Date: 2000/11/06 22:30:46 $
 * $Locker:  $
 * $Revision: 1.1 $
 *
 * Created:  3/2/00 by Charles Chiou
 * Comments:
 * Status:
 * Bugs:
 *
 * Copyright (c) 1999 Selfsoft Corp. All rights reserved.
 *************************************************************************/

#ifndef _BIGINTEGER_HXX_
#define _BIGINTEGER_HXX_

#include "BaseObject.hxx"

__BEGIN_NAMESPACE(SELFSOFT);

class UTILAPI BigInteger : public BaseObject {

    DECLARE_RUNTIME_SERIALIZABLE(BigInteger);

public:

    BigInteger();
    BigInteger(const BigInteger &bigInteger);
    BigInteger(const char *string);
    ~BigInteger();

    // Assignment operators
    BigInteger &operator=(const BigInteger &bigInteter);

    // Comparison operators
    boolean operator==(const BigInteger &bigInteger);
    boolean operator!=(const BigInteger &bigInteger);
    boolean operator>(const BigInteger &bigInteger);
    boolean operator>=(const BigInteger &bigInteger);
    boolean operator<(const BigInteger &bigInteger);
    boolean operator<=(const BigInteger &bigInteger);

    BigInteger operator+(const BigInteger &bigInteger) const;
    BigInteger operator-(const BigInteger &bigInteger) const;
    BigInteger operator*(const BigInteger &bigInteger) const;
    BigInteger operator/(const BigInteger &bigInteger) const;
    BigInteger operator%(const BigInteger &bigInteger) const;
  
    BigInteger &operator+=(const BigInteger &bigInteger);
    BigInteger &operator-=(const BigInteger &bigInteger);
    BigInteger &operator*=(const BigInteger &bigInteger);
    BigInteger &operator/=(const BigInteger &bigInteger);
    BigInteger &operator%=(const BigInteger &bigInteger);

private:

};

// Inline functions

inline BigInteger &BigInteger::operator+=(const BigInteger &bigInteger) {
    *this = operator+(bigInteger);
    return *this;
}

inline BigInteger &BigInteger::operator-=(const BigInteger &bigInteger) {
    *this = operator-(bigInteger);
    return *this;
}

inline BigInteger &BigInteger::operator*=(const BigInteger &bigInteger) {
    *this = operator*(bigInteger);
    return *this;
}

inline BigInteger &BigInteger::operator/=(const BigInteger &bigInteger) {
    *this = operator/(bigInteger);
    return *this;
}

inline BigInteger &BigInteger::operator%=(const BigInteger &bigInteger) {
    *this = operator%(bigInteger);
    return *this;
}

__END_NAMESPACE(SELFSOFT);

#endif

/*
 * Local variables:
 * mode: C++
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
