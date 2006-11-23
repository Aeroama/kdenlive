/***************************************************************************
                          capturemonitor  -  description
                             -------------------
    begin                : Sun Jun 12 2005
    copyright            : (C) 2005 by Jason Wood
    email                : jasonwood@blueyonder.co.uk
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#include "capturemonitor.h"

#include <qtoolbutton.h>

#include <kled.h>
#include <klocale.h>
#include <kio/netaccess.h>
#include <kstandarddirs.h>

#include <kdenlivedoc.h>
#include <kmmrecpanel.h>
#include <kurlrequester.h>
#include <klistview.h>
#include <kiconloader.h>

namespace Gui {

    CaptureMonitor::CaptureMonitor(KdenliveApp * app, QWidget * parent,
	const char *name):KMonitor(parent, name),
	m_app(app), m_screenHolder(new QVBox(this,name)), m_screen(new QWidget(m_screenHolder, name)),
	m_recPanel(new KMMRecPanel(app->getDocument(), this, name)), captureProcess(0), hasCapturedFiles(false), m_tmpFolder(0)
    {

	m_screen->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);
	m_screen->setBackgroundMode(Qt::PaletteDark);

	if (KStandardDirs::findExe("dvgrab") == QString::null ||KStandardDirs::findExe("ffplay") == QString::null) {
		m_recPanel->setEnabled(false);
		QLabel *lb = new QLabel(i18n("<b>The programs dvgrab or ffplay are missing</b>.<br>Firewire capture will be disabled until you install them."), m_screenHolder);
		lb->setPaletteBackgroundColor(Qt::red);
		lb->setMargin(5);
		lb->show();
	}

	connect(m_recPanel, SIGNAL(activateMonitor()), this,  SLOT(activateMonitor()));
	connect(m_recPanel, SIGNAL(stopDevice()), this, SLOT(slotStop()));
	connect(m_recPanel, SIGNAL(playDevice()), this, SLOT(slotPlay()));
	connect(m_recPanel, SIGNAL(recDevice()), this, SLOT(slotRec()));
	connect(m_recPanel, SIGNAL(forwardDevice()), this, SLOT(slotFastForward()));
	connect(m_recPanel, SIGNAL(stepForwardDevice()), this, SLOT(slotForward()));
	connect(m_recPanel, SIGNAL(rewindDevice()), this, SLOT(slotRewind()));
	connect(m_recPanel, SIGNAL(stepRewindDevice()), this, SLOT(slotReverse()));
    } 

    CaptureMonitor::~CaptureMonitor() {
	if (captureProcess) delete captureProcess;
    }
    
    void CaptureMonitor::exportCurrentFrame(KURL url) const {
	// TODO FIXME
    } 

    KMMEditPanel *CaptureMonitor::editPanel() const {
	// TODO FIXME
	return 0;
    } 
    
    KMMScreen *CaptureMonitor::screen() const {
	// TODO FIXME
	return 0;
    } 

    void CaptureMonitor::slotClickMonitor() {
	emit monitorClicked(this);
    }

    void CaptureMonitor::activateMonitor() {
	m_app->activateMonitor(this);
    }

    void CaptureMonitor::slotSetActive() {
        m_recPanel->rendererConnected();
    }
    
    DocClipRef *CaptureMonitor::clip() const {
	return 0;
    } 
    
    void CaptureMonitor::slotSetupScreen() {
	//m_screen->setCapture();
    }

    void CaptureMonitor::slotRewind() {
	if (!captureProcess) slotInit();
	captureProcess->writeStdin("a", 1);
    }

    void CaptureMonitor::slotReverse() {
	if (!captureProcess) slotInit();
	captureProcess->writeStdin("j", 1);
    }


void CaptureMonitor::displayCapturedFiles()
{
	KDialogBase *dia = new KDialogBase(  KDialogBase::Swallow, i18n("Captured Clips"), KDialogBase::Ok | KDialogBase::Cancel, KDialogBase::Ok, this, "captured_clips", true);

	KListView *lv = new KListView(dia);
	lv->addColumn(i18n("Add"));
	lv->addColumn("original_name",0);
	lv->addColumn(i18n("Clip Name"));
	lv->setItemsRenameable(true);
	lv->setResizeMode(QListView::LastColumn);
	lv->setRenameable(2, true);
	lv->setRenameable(0, false);
	lv->setAllColumnsShowFocus(true);

	QStringList more;
    	QStringList::Iterator it;

        QDir dir( KURL(m_tmpFolder).path() );
        more = dir.entryList( QDir::Files );
        for ( it = more.begin() ; it != more.end() ; ++it ){
		QPixmap p = m_app->getDocument()->renderer()->getVideoThumbnail(KURL(m_tmpFolder + (*it)), 1, 60, 40);
		//QPixmap p = QImage(selectedFolder() + "/" + (*it)).smoothScale(50, 40);
		QCheckListItem *item = new QCheckListItem(lv, QString::null, QCheckListItem::CheckBox);
		item->setPixmap(0, p);
		item->setText(1, (*it));
		item->setText(2, (*it));
		((QCheckListItem*)item)->setOn(true);
	    }
	lv->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	dia->setMainWidget(lv);
	dia->setMinimumSize(400, 240);
	dia->adjustSize();
	if (dia->exec() == QDialog::Accepted) {

	QListViewItemIterator it( lv );
    	for ( ; it.current(); ++it )
            if ( ( (QCheckListItem*)it.current() )->isOn() ) {
		// move selected files to our project folder
                QString source = m_tmpFolder + it.current()->text( 1 );
		QString dest = KdenliveSettings::currentdefaultfolder() + "/" + it.current()->text( 2 );
		KIO::NetAccess::move(KURL(source), KURL(dest), this);
		m_app->insertClipFromUrl(dest);
	    }
	}
	KIO::NetAccess::del(m_tmpFolder, this);
	m_tmpFolder = QString::null;

}

    void CaptureMonitor::slotStop(KProcess *p) {
	if (captureProcess) {

        if (m_screen) {
	    delete m_screen;
	    m_screen = new QWidget(m_screenHolder, name());
	    m_screen->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);
	    m_screen->setBackgroundMode(Qt::PaletteDark);
	    m_screen->show();
	}
	    captureProcess->writeStdin("q", 1);
	    if (captureProcess) delete captureProcess;
	    captureProcess = 0;
	}
	m_recPanel->rendererDisconnected();
	if (hasCapturedFiles) displayCapturedFiles();
    }

    void CaptureMonitor::slotInit() {
	if (captureProcess) slotStop();
	captureProcess = new KProcess();
	if (!m_tmpFolder.isEmpty()) KIO::NetAccess::del(m_tmpFolder, this);
	m_tmpFolder = locateLocal("tmp", "dvcapture/", true);
	captureProcess->setWorkingDirectory(m_tmpFolder);
        captureProcess->setUseShell(true);
        captureProcess->setEnvironment("SDL_WINDOWID", QString::number(m_screen->winId()));
        *captureProcess<<"dvgrab";
        *captureProcess<<"--format"<<"raw"<<"-i"<<"capture"<<"-";
        *captureProcess<<"|"<<"ffplay"<<"-f"<<"dv"<<"-x"<<QString::number(m_screen->width())<<"-y"<<QString::number(m_screen->height())<<"-";
	captureProcess->start(KProcess::NotifyOnExit, KProcess::Stdin);
        connect(captureProcess, SIGNAL(processExited(KProcess *)), this, SLOT(slotStop(KProcess *)));
	m_recPanel->rendererConnected();
	hasCapturedFiles = false;
    }

    void CaptureMonitor::slotPlay() {
	if (!captureProcess) slotInit();
	captureProcess->writeStdin(" ", 1);

	/*if (captureProcess) return;
	captureProcess = new KProcess();
        captureProcess->setUseShell(true);
        captureProcess->setEnvironment("SDL_WINDOWID", QString::number(m_screen->captureId()));
        *captureProcess<<"dvgrab";
        *captureProcess<<"--format"<<"raw"<<"-"; //<<"testme-";
        *captureProcess<<"|"<<"ffplay"<<"-f"<<"dv"<<"-";
	captureProcess->start();*/
    }

    void CaptureMonitor::slotRec() {
	if (!captureProcess) slotInit();
	captureProcess->writeStdin("c\n", 3);
	hasCapturedFiles = true;
    }


    void CaptureMonitor::slotForward() {
	if (!captureProcess) slotInit();
	captureProcess->writeStdin("l", 1);
    }

    void CaptureMonitor::slotFastForward() {
	if (!captureProcess) slotInit();
	captureProcess->writeStdin("z", 1);
    }

}

#include "capturemonitor.moc"
