/*************************************************************************
 * $Id: OctiGameSelectDialog.cxx,v 1.1 2001/08/09 04:30:27 samurai Exp $
 * $Author: samurai $
 * $Date: 2001/08/09 04:30:27 $
 * $Locker:  $
 * $Revision: 1.1 $
 *
 * Created: 7/5/01 by Charles Chiou
 * Comments:
 * Status:
 * Bugs:
 *
 * Work done while a student at Yale University.
 * Copyright (c) 2001 Selfsoft Corp. All rights reserved.
 *************************************************************************/

#include <qbuttongroup.h>
#include <qcheckbox.h>
#include <qcombobox.h>
#include <qgroupbox.h>
#include <qpushbutton.h>
#include <qradiobutton.h>
#include <qlayout.h>
#include <qvariant.h>
#include <qtooltip.h>
#include <qwhatsthis.h>
#include "OctiGameSelectDialog.hxx"

__BEGIN_NAMESPACE(SELFSOFT);

OctiGameSelectDialog::OctiGameSelectDialog(QWidget* parent, const char* name, bool modal, WFlags fl) : OctiGameSelectDialogBase( parent, name, modal, fl) {
  timeLimitComboBox->insertItem( tr( "10 min." ) );
  timeLimitComboBox->insertItem( tr( "15 min." ) );
  timeLimitComboBox->insertItem( tr( "20 min." ) );
  timeLimitComboBox->insertItem( tr( "30 min." ) );
  timeLimitComboBox->insertItem( tr( "45 min." ) );
  timeLimitComboBox->insertItem( tr( "60 min." ) );
}

OctiGameSelectDialog::~OctiGameSelectDialog() {

}

OctiGameMode OctiGameSelectDialog::getGameMode() const {
  return twoPlayerButton->isOn() ? GAME_MODE_2 : GAME_MODE_4; 
}

OctiGameBases OctiGameSelectDialog::getGameBases() const {
  return threeBaseGameButton->isOn() ? GAME_BASES_3 : GAME_BASES_1;
}

boolean OctiGameSelectDialog::useSuperProngs() const {
  return superProngCheckBox->isOn();
}

boolean OctiGameSelectDialog::edgeless() const {
  return edgelessCheckBox->isOn();
}

boolean OctiGameSelectDialog::useTimeLimit() const {
  return useTimeLimitCheckBox->isOn();
}

int OctiGameSelectDialog::getTimeLimit() const {
  if(useTimeLimitCheckBox->isOn()) {
    switch(timeLimitComboBox->currentItem()) {
    case 0:   return  600; break;  // 10 min.
    case 1:   return  900; break;  // 15 min.
    case 2:   return 1200; break;  // 20 min.
    case 3:   return 1800; break;  // 30 min.
    case 4:   return 2700; break;  // 45 min.
    case 5:   return 3600; break;  // 60 min.
    default:  return 0;
    }
  }

  return 0;
}

__END_NAMESPACE(SELFSOFT);
