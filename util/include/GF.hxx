/*************************************************************************
 * $Id: GF.hxx,v 1.1 2000/04/27 05:37:02 cychiou Exp $
 * $Author: cychiou $
 * $Date: 2000/04/27 05:37:02 $
 * $Locker:  $
 * $Revision: 1.1 $
 *
 * Created:  3/15/00 by Charles Chiou
 * Comments: Galois field arithmetics.
 * Status:
 * Bugs:
 *
 * Copyright (c) 1999 Selfsoft Corp. All rights reserved.
 *************************************************************************/

#ifndef _GF_HXX_
#define _GF_HXX_

#include "BaseObject.hxx"

__BEGIN_NAMESPACE(SELFSOFT);

class UTILAPI Gf : public BaseObject {

    DECLARE_RUNTIME_SERIALIZBLE(Gf);
  
public:

    Gf();
    ~Gf();

private:

};

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
