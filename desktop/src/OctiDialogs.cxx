/*
 * OctiDialogs.cxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#include <algorithm>
#include <cmath>
#include <fstream>
#include <QAbstractItemView>
#include <QDesktopServices>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QIcon>
#include <QPainter>
#include <QPixmap>
#include <QUrl>
#include <QVBoxLayout>
#include "OctiCore.hxx"
#include "OctiDialogs.hxx"
#include "OctiMainWindow.hxx"

namespace octi {

namespace {

QIcon makeProviderIcon(const QString &provider)
{
    QPixmap pix(28, 28);
    pix.fill(Qt::transparent);
    QPainter p(&pix);
    paintProviderIcon(p, provider, QRectF(2.0, 2.0, 24.0, 24.0));
    p.end();
    return QIcon(pix);
}

std::string toUpperCopy(std::string s)
{
    for (std::size_t i = 0; i < s.size(); ++i) {
        if (s[i] >= 'a' && s[i] <= 'z') {
            s[i] = static_cast<char>(s[i] - 'a' + 'A');
        }
    }
    return s;
}

} // namespace

/* ==================================================================== */
/* 1. SsoOnboardingDialog                                               */
/* ==================================================================== */

SsoOnboardingDialog::SsoOnboardingDialog(OctiNetworkClient *netClient,
                                         ClientConfigStore *configStore,
                                         ThemeId theme,
                                         QWidget *parent)
    : QDialog(parent),
      _netClient(netClient),
      _configStore(configStore),
      _theme(theme),
      _dispatcher(this),
      _serverBadgeLabel(nullptr),
      _statusLabel(nullptr),
      _providerCombo(nullptr),
      _tokenEdit(nullptr),
      _handleEdit(nullptr),
      _signInBtn(nullptr),
      _step2Container(nullptr),
      _onboardingHandleEdit(nullptr),
      _completeOnboardingBtn(nullptr),
      _signedIn(false),
      _step2Active(false)
{
    setWindowTitle("OCTI — Sign In & Onboarding");
    setMinimumSize(520, 480);

    QVBoxLayout *root = new QVBoxLayout(this);
    root->setSpacing(12);
    root->setContentsMargins(20, 20, 20, 20);

    QLabel *title = new QLabel("OCTI Online Authentication", this);
    title->setStyleSheet("font-size: 18px; font-weight: bold;");
    root->addWidget(title);

    _serverBadgeLabel = nullptr;

    QGroupBox *browserGroup = new QGroupBox("1-Click Browser SSO Sign-In", this);
    QHBoxLayout *browserRow = new QHBoxLayout(browserGroup);
    static const struct {
        const char *id;
        const char *label;
    } kProviders[5] = {
        {"google",    "Google"},
        {"apple",     "Apple"},
        {"microsoft", "Microsoft"},
        {"facebook",  "Facebook"},
        {"line",      "LINE"},
    };
    for (int i = 0; i < 5; ++i) {
        const std::string provId = kProviders[i].id;
        QPushButton *btn = new QPushButton(
            QString::fromUtf8(kProviders[i].label), browserGroup);
        btn->setIcon(makeProviderIcon(QString::fromUtf8(kProviders[i].id)));
        QObject::connect(btn, &QPushButton::clicked, [this, provId]() {
            startBrowserLoopbackFlow(provId, true);
        });
        browserRow->addWidget(btn);
    }
    root->addWidget(browserGroup);

    QGroupBox *manualGroup = new QGroupBox("Direct / Token Sign-In", this);
    QFormLayout *manualForm = new QFormLayout(manualGroup);
    _providerCombo = new QComboBox(manualGroup);
    for (int i = 0; i < 5; ++i) {
        _providerCombo->addItem(
            makeProviderIcon(QString::fromUtf8(kProviders[i].id)),
            QString::fromUtf8(kProviders[i].label),
            QString::fromUtf8(kProviders[i].id));
    }
    _tokenEdit = new QLineEdit(manualGroup);
    _tokenEdit->setObjectName("ssoTokenEdit");
    _tokenEdit->setPlaceholderText("Paste OIDC id_token (or Facebook access_token)...");

    _handleEdit = new QLineEdit(manualGroup);
    _handleEdit->setObjectName("desiredHandleEdit");
    _handleEdit->setPlaceholderText("Optional initial handle (e.g. GrandmasterA)");

    _signInBtn = new QPushButton("Sign In", manualGroup);
    QObject::connect(_signInBtn, &QPushButton::clicked, [this]() {
        const std::string prov =
            _providerCombo->currentData().toString().toStdString();
        const std::string tok = _tokenEdit->text().trimmed().toStdString();
        const std::string hdl = _handleEdit->text().trimmed().toStdString();
        signInWithToken(prov, tok, hdl, false);
    });

    manualForm->addRow("SSO Provider:", _providerCombo);
    manualForm->addRow("ID Token:", _tokenEdit);
    manualForm->addRow("Initial Handle:", _handleEdit);
    manualForm->addRow("", _signInBtn);
    root->addWidget(manualGroup);

    _step2Container = new QGroupBox(
        "Step 2: Welcome! Customize Your Player Handle", this);
    QVBoxLayout *step2Layout = new QVBoxLayout(_step2Container);
    _onboardingHandleEdit = new QLineEdit(_step2Container);
    _onboardingHandleEdit->setObjectName("onboardingHandleEdit");
    _onboardingHandleEdit->setPlaceholderText("Choose your public OCTI handle...");
    _completeOnboardingBtn = new QPushButton(
        "Complete Onboarding", _step2Container);
    QObject::connect(_completeOnboardingBtn, &QPushButton::clicked, [this]() {
        finalizeOnboardingHandle(
            _onboardingHandleEdit->text().trimmed().toStdString());
    });
    step2Layout->addWidget(_onboardingHandleEdit);
    step2Layout->addWidget(_completeOnboardingBtn);
    _step2Container->setVisible(false);
    root->addWidget(_step2Container);

    _statusLabel = new QLabel("Ready to authenticate.", this);
    _statusLabel->setWordWrap(true);
    root->addWidget(_statusLabel);

    QPushButton *closeBtn = new QPushButton("Close", this);
    QObject::connect(closeBtn, &QPushButton::clicked, [this]() {
        reject();
    });
    root->addWidget(closeBtn);

    setTheme(_theme);
}

SsoOnboardingDialog::~SsoOnboardingDialog()
{
    stopBrowserLoopbackFlow();
}

void SsoOnboardingDialog::setTheme(ThemeId theme)
{
    _theme = theme;
    setStyleSheet(buildAppStyleSheet(_theme));
}

ThemeId SsoOnboardingDialog::theme() const
{
    return _theme;
}

QString SsoOnboardingDialog::serverBadgeText() const
{
    return _serverBadgeLabel ? _serverBadgeLabel->text() : QString();
}

bool SsoOnboardingDialog::hasEditableServerOrPortField() const
{
    const QList<QLineEdit *> edits = findChildren<QLineEdit *>();
    for (int i = 0; i < edits.size(); ++i) {
        const QLineEdit *ed = edits[i];
        if (!ed) {
            continue;
        }
        const QString name = ed->objectName().toLower();
        const QString ph = ed->placeholderText().toLower();
        if (name.contains("server") || name.contains("host") ||
            name.contains("port") || ph.contains("server") ||
            ph.contains("port") || ph.contains("trumpet")) {
            return true;
        }
    }
    const QList<QSpinBox *> spins = findChildren<QSpinBox *>();
    if (!spins.isEmpty()) {
        return true;
    }
    return false;
}

bool SsoOnboardingDialog::startBrowserLoopbackFlow(const std::string &provider,
                                                   bool openSystemBrowser)
{
    stopBrowserLoopbackFlow();
    _lastProvider = provider;
    std::string err;
    bool ok = _loopbackListener.start(
        [this](const std::string &cbProvider, const std::string &cbToken) {
            std::string resolvedProv =
                cbProvider.empty() ? _lastProvider : cbProvider;
            _dispatcher.post([this, resolvedProv, cbToken]() {
                std::string desired =
                    _handleEdit ? _handleEdit->text().trimmed().toStdString()
                                : std::string();
                signInWithToken(resolvedProv, cbToken, desired, false);
            });
        },
        err);
    if (!ok) {
        if (_statusLabel) {
            _statusLabel->setText(QString::fromStdString(err));
        }
        return false;
    }
    if (_statusLabel) {
        _statusLabel->setText(
            QString("Listening for %1 callback on %2")
                .arg(QString::fromStdString(provider))
                .arg(QString::fromStdString(_loopbackListener.callbackUrl())));
    }
    if (openSystemBrowser) {
        QString url = QString("https://%1:%2/api/v1/auth/providers?provider=%3&redirect_uri=%4")
            .arg(QString::fromUtf8(OCTI_SERVER_HOST))
            .arg(OCTI_SERVER_PORT)
            .arg(QString::fromStdString(provider))
            .arg(QString::fromStdString(_loopbackListener.callbackUrl()));
        QDesktopServices::openUrl(QUrl(url));
    }
    return true;
}

void SsoOnboardingDialog::stopBrowserLoopbackFlow()
{
    _loopbackListener.stop();
}

int SsoOnboardingDialog::loopbackBoundPort() const
{
    return _loopbackListener.boundPort();
}

std::string SsoOnboardingDialog::loopbackCallbackUrl() const
{
    return _loopbackListener.callbackUrl();
}

bool SsoOnboardingDialog::pollLoopbackCallback(int timeoutMs)
{
    std::string prov;
    std::string tok;
    if (_loopbackListener.waitForCallback(timeoutMs, prov, tok)) {
        _dispatcher.drain();
        if (!_signedIn && !tok.empty()) {
            std::string resolvedProv = prov.empty() ? _lastProvider : prov;
            std::string desired =
                _handleEdit ? _handleEdit->text().trimmed().toStdString()
                            : std::string();
            signInWithToken(resolvedProv, tok, desired, false);
        }
        return _signedIn;
    }
    _dispatcher.drain();
    return _signedIn;
}

ApiResult<SsoLoginData> SsoOnboardingDialog::signInWithToken(
    const std::string &provider,
    const std::string &token,
    const std::string &desiredHandle,
    bool isAccessToken)
{
    ApiResult<SsoLoginData> res;
    if (!_netClient) {
        res.errorMessage = "Network client is null";
        if (_statusLabel) {
            _statusLabel->setText("Network client unavailable.");
        }
        return res;
    }
    res = _netClient->loginWithSso(
        provider, token, desiredHandle, isAccessToken);
    if (!res.ok) {
        if (_statusLabel) {
            _statusLabel->setText(
                QString("Sign-in failed (%1): %2")
                    .arg(res.httpStatus)
                    .arg(QString::fromStdString(res.errorMessage)));
        }
        return res;
    }

    _signedIn = true;
    _lastProvider = provider;
    _loggedInProfile = res.data.profile;
    persistLoginToConfig(provider, res.data.sessionToken, _loggedInProfile);

    if (res.data.isNewAccount || res.httpStatus == 201) {
        _step2Active = true;
        if (_step2Container) {
            _step2Container->setVisible(true);
        }
        if (_onboardingHandleEdit) {
            _onboardingHandleEdit->setText(
                QString::fromStdString(_loggedInProfile.handle));
        }
        if (_statusLabel) {
            _statusLabel->setText(
                QString("Account created (201) for %1! Customize your handle below.")
                    .arg(QString::fromStdString(_loggedInProfile.handle)));
        }
    } else {
        _step2Active = false;
        if (_step2Container) {
            _step2Container->setVisible(false);
        }
        if (_statusLabel) {
            _statusLabel->setText(
                QString("Signed in as %1 (2P Elo %2).")
                    .arg(QString::fromStdString(_loggedInProfile.handle))
                    .arg(_loggedInProfile.elo2p));
        }
        if (_onSignedInCallback) {
            _onSignedInCallback(_loggedInProfile);
        }
        accept();
    }
    return res;
}

bool SsoOnboardingDialog::isOnboardingStep2Active() const
{
    return _step2Active;
}

ApiResult<PlayerProfileInfo> SsoOnboardingDialog::finalizeOnboardingHandle(
    const std::string &newHandle)
{
    ApiResult<PlayerProfileInfo> res;
    if (!_netClient) {
        res.errorMessage = "Network client is null";
        return res;
    }
    res = _netClient->updateMyProfile(newHandle);
    if (!res.ok) {
        if (_statusLabel) {
            _statusLabel->setText(
                QString("Could not update handle (%1): %2")
                    .arg(res.httpStatus)
                    .arg(QString::fromStdString(res.errorMessage)));
        }
        return res;
    }
    _loggedInProfile = res.data;
    _step2Active = false;
    if (_step2Container) {
        _step2Container->setVisible(false);
    }
    persistLoginToConfig(
        _lastProvider, _netClient->sessionToken(), _loggedInProfile);
    if (_statusLabel) {
        _statusLabel->setText(
            QString("Onboarding complete! Welcome, %1.")
                .arg(QString::fromStdString(_loggedInProfile.handle)));
    }
    if (_onSignedInCallback) {
        _onSignedInCallback(_loggedInProfile);
    }
    accept();
    return res;
}

bool SsoOnboardingDialog::isSignedIn() const
{
    return _signedIn;
}

const PlayerProfileInfo &SsoOnboardingDialog::loggedInProfile() const
{
    return _loggedInProfile;
}

QString SsoOnboardingDialog::statusText() const
{
    return _statusLabel ? _statusLabel->text() : QString();
}

void SsoOnboardingDialog::setOnSignedInCallback(
    std::function<void(const PlayerProfileInfo &)> cb)
{
    _onSignedInCallback = std::move(cb);
}

void SsoOnboardingDialog::persistLoginToConfig(
    const std::string &provider,
    const std::string &sessionToken,
    const PlayerProfileInfo &profile)
{
    if (!_configStore) {
        return;
    }
    ClientPreferences &prefs = _configStore->preferences();
    prefs.sessionToken = sessionToken;
    prefs.playerUuid = profile.uuid;
    prefs.handle = profile.handle;
    prefs.displayName = profile.displayName;
    prefs.avatarUrl = profile.avatarUrl;
    prefs.preferredProvider = provider;
    std::string ignoredErr;
    _configStore->save(ignoredErr);
}

/* ==================================================================== */
/* 2. AccountLinkDialog                                                 */
/* ==================================================================== */

AccountLinkDialog::AccountLinkDialog(OctiNetworkClient *netClient,
                                     ClientConfigStore *configStore,
                                     ThemeId theme,
                                     QWidget *parent)
    : QDialog(parent),
      _netClient(netClient),
      _configStore(configStore),
      _theme(theme),
      _identitiesTable(nullptr),
      _providerCombo(nullptr),
      _tokenEdit(nullptr),
      _linkBtn(nullptr),
      _unlinkBtn(nullptr),
      _errorBannerLabel(nullptr),
      _conflictBox(nullptr),
      _conflictBannerLabel(nullptr),
      _switchConflictBtn(nullptr),
      _cancelConflictBtn(nullptr),
      _hasConflict(false),
      _conflictIsAccessToken(false)
{
    setWindowTitle("OCTI — Linked SSO Identities");
    setMinimumSize(560, 460);

    QVBoxLayout *root = new QVBoxLayout(this);
    root->setSpacing(10);

    QLabel *hdr = new QLabel("Manage Linked SSO Providers", this);
    hdr->setStyleSheet("font-size: 16px; font-weight: bold;");
    root->addWidget(hdr);

    _identitiesTable = new QTableWidget(0, 3, this);
    _identitiesTable->setHorizontalHeaderLabels(
        QStringList() << "Provider" << "Email" << "Display Hint");
    _identitiesTable->horizontalHeader()->setSectionResizeMode(
        QHeaderView::Stretch);
    _identitiesTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    _identitiesTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    root->addWidget(_identitiesTable);

    QGroupBox *linkGroup = new QGroupBox("Link or Unlink Provider", this);
    QFormLayout *form = new QFormLayout(linkGroup);
    _providerCombo = new QComboBox(linkGroup);
    static const char *kProvs[5] = {
        "google", "apple", "microsoft", "facebook", "line"
    };
    for (int i = 0; i < 5; ++i) {
        _providerCombo->addItem(
            makeProviderIcon(QString::fromUtf8(kProvs[i])),
            QString::fromUtf8(kProvs[i]),
            QString::fromUtf8(kProvs[i]));
    }
    _tokenEdit = new QLineEdit(linkGroup);
    _tokenEdit->setPlaceholderText("ID Token to link...");

    QHBoxLayout *btnRow = new QHBoxLayout();
    _linkBtn = new QPushButton("Link Provider", linkGroup);
    _unlinkBtn = new QPushButton("Unlink Provider", linkGroup);
    btnRow->addWidget(_linkBtn);
    btnRow->addWidget(_unlinkBtn);

    QObject::connect(_linkBtn, &QPushButton::clicked, [this]() {
        linkProviderIdentity(
            _providerCombo->currentData().toString().toStdString(),
            _tokenEdit->text().trimmed().toStdString(),
            false);
    });
    QObject::connect(_unlinkBtn, &QPushButton::clicked, [this]() {
        unlinkProviderIdentity(
            _providerCombo->currentData().toString().toStdString());
    });

    form->addRow("Provider:", _providerCombo);
    form->addRow("Token:", _tokenEdit);
    form->addRow("", btnRow);
    root->addWidget(linkGroup);

    _errorBannerLabel = new QLabel("", this);
    _errorBannerLabel->setWordWrap(true);
    _errorBannerLabel->setVisible(false);
    root->addWidget(_errorBannerLabel);

    _conflictBox = new QGroupBox("Identity Already Linked (409 Conflict)", this);
    QVBoxLayout *cLayout = new QVBoxLayout(_conflictBox);
    _conflictBannerLabel = new QLabel("", _conflictBox);
    _conflictBannerLabel->setWordWrap(true);
    cLayout->addWidget(_conflictBannerLabel);

    QHBoxLayout *cBtns = new QHBoxLayout();
    _switchConflictBtn = new QPushButton(
        "Switch to Existing Account", _conflictBox);
    _cancelConflictBtn = new QPushButton("Cancel", _conflictBox);
    cBtns->addWidget(_switchConflictBtn);
    cBtns->addWidget(_cancelConflictBtn);
    cLayout->addLayout(cBtns);

    QObject::connect(_switchConflictBtn, &QPushButton::clicked, [this]() {
        confirmSwitchToConflictAccount();
    });
    QObject::connect(_cancelConflictBtn, &QPushButton::clicked, [this]() {
        cancelConflictSwitch();
    });

    _conflictBox->setVisible(false);
    root->addWidget(_conflictBox);

    QPushButton *closeBtn = new QPushButton("Done", this);
    QObject::connect(closeBtn, &QPushButton::clicked, [this]() {
        accept();
    });
    root->addWidget(closeBtn);

    setTheme(_theme);
    refreshProfile();
}

AccountLinkDialog::~AccountLinkDialog() = default;

void AccountLinkDialog::setTheme(ThemeId theme)
{
    _theme = theme;
    setStyleSheet(buildAppStyleSheet(_theme));
}

ThemeId AccountLinkDialog::theme() const
{
    return _theme;
}

bool AccountLinkDialog::refreshProfile()
{
    if (!_netClient || _netClient->sessionToken().empty()) {
        return false;
    }
    auto res = _netClient->fetchMyProfile();
    if (!res.ok) {
        return false;
    }
    _currentProfile = res.data;
    _identities = res.data.identities;
    rebuildIdentitiesTable();
    return true;
}

const PlayerProfileInfo &AccountLinkDialog::currentProfile() const
{
    return _currentProfile;
}

const std::vector<LinkedIdentityInfo> &AccountLinkDialog::linkedIdentities() const
{
    return _identities;
}

void AccountLinkDialog::rebuildIdentitiesTable()
{
    if (!_identitiesTable) {
        return;
    }
    _identitiesTable->setRowCount(static_cast<int>(_identities.size()));
    for (int i = 0; i < static_cast<int>(_identities.size()); ++i) {
        const LinkedIdentityInfo &id = _identities[static_cast<std::size_t>(i)];
        _identitiesTable->setItem(
            i, 0, new QTableWidgetItem(QString::fromStdString(id.provider)));
        _identitiesTable->setItem(
            i, 1, new QTableWidgetItem(QString::fromStdString(id.providerEmail)));
        _identitiesTable->setItem(
            i, 2, new QTableWidgetItem(QString::fromStdString(id.displayHint)));
    }
}

ApiResult<LinkIdentityData> AccountLinkDialog::linkProviderIdentity(
    const std::string &provider,
    const std::string &token,
    bool isAccessToken)
{
    ApiResult<LinkIdentityData> res;
    if (!_netClient) {
        res.errorMessage = "Network client is null";
        return res;
    }
    res = _netClient->linkIdentity(provider, token, isAccessToken);
    if (res.ok) {
        _hasConflict = false;
        if (_conflictBox) {
            _conflictBox->setVisible(false);
        }
        if (_errorBannerLabel) {
            _errorBannerLabel->clear();
            _errorBannerLabel->setVisible(false);
        }
        if (!res.data.identities.empty()) {
            _identities = res.data.identities;
            rebuildIdentitiesTable();
        } else {
            refreshProfile();
        }
        return res;
    }

    if (res.httpStatus == 409 && res.data.conflictAlreadyLinked) {
        _hasConflict = true;
        _conflictProvider = provider;
        _conflictToken = token;
        _conflictIsAccessToken = isAccessToken;
        _conflictProfile = res.data.existingProfile;
        if (_conflictBannerLabel) {
            _conflictBannerLabel->setText(
                QString("This %1 identity is already linked to account '%2' (%3). "
                        "Would you like to switch to that existing account?")
                    .arg(QString::fromStdString(provider))
                    .arg(QString::fromStdString(_conflictProfile.handle))
                    .arg(QString::fromStdString(_conflictProfile.uuid)));
        }
        if (_conflictBox) {
            _conflictBox->setVisible(true);
        }
    } else {
        if (_errorBannerLabel) {
            _errorBannerLabel->setText(
                QString("Link failed (%1): %2")
                    .arg(res.httpStatus)
                    .arg(QString::fromStdString(res.errorMessage)));
            _errorBannerLabel->setVisible(true);
        }
    }
    return res;
}

bool AccountLinkDialog::hasPendingConflict() const
{
    return _hasConflict;
}

const PlayerProfileInfo &AccountLinkDialog::conflictExistingProfile() const
{
    return _conflictProfile;
}

ApiResult<SsoLoginData> AccountLinkDialog::confirmSwitchToConflictAccount(
    const std::string &provider,
    const std::string &token)
{
    ApiResult<SsoLoginData> res;
    if (!_netClient) {
        res.errorMessage = "Network client is null";
        return res;
    }
    const std::string useProv =
        provider.empty() ? _conflictProvider : provider;
    const std::string useTok =
        token.empty() ? _conflictToken : token;
    res = _netClient->loginWithSso(
        useProv, useTok, "", _conflictIsAccessToken);
    if (res.ok) {
        _hasConflict = false;
        if (_conflictBox) {
            _conflictBox->setVisible(false);
        }
        _currentProfile = res.data.profile;
        _identities = res.data.identities;
        if (_identities.empty()) {
            refreshProfile();
        } else {
            rebuildIdentitiesTable();
        }
        if (_configStore) {
            ClientPreferences &prefs = _configStore->preferences();
            prefs.sessionToken = res.data.sessionToken;
            prefs.playerUuid = _currentProfile.uuid;
            prefs.handle = _currentProfile.handle;
            prefs.displayName = _currentProfile.displayName;
            prefs.avatarUrl = _currentProfile.avatarUrl;
            prefs.preferredProvider = useProv;
            std::string ignoredErr;
            _configStore->save(ignoredErr);
        }
        if (_onAccountSwitchedCallback) {
            _onAccountSwitchedCallback(_currentProfile);
        }
    }
    return res;
}

void AccountLinkDialog::cancelConflictSwitch()
{
    _hasConflict = false;
    _conflictProvider.clear();
    _conflictToken.clear();
    _conflictProfile = PlayerProfileInfo();
    if (_conflictBannerLabel) {
        _conflictBannerLabel->clear();
    }
    if (_conflictBox) {
        _conflictBox->setVisible(false);
    }
}

ApiResult<UnlinkIdentityData> AccountLinkDialog::unlinkProviderIdentity(
    const std::string &provider)
{
    ApiResult<UnlinkIdentityData> res;
    if (!_netClient) {
        res.errorMessage = "Network client is null";
        return res;
    }
    res = _netClient->unlinkIdentity(provider);
    if (res.ok) {
        if (_errorBannerLabel) {
            _errorBannerLabel->clear();
            _errorBannerLabel->setVisible(false);
        }
        if (!res.data.identities.empty()) {
            _identities = res.data.identities;
            rebuildIdentitiesTable();
        } else {
            refreshProfile();
        }
        return res;
    }
    if (_errorBannerLabel) {
        if (res.httpStatus == 400) {
            _errorBannerLabel->setText(
                QString("Cannot unlink last identity (%1): %2")
                    .arg(QString::fromStdString(provider))
                    .arg(QString::fromStdString(res.errorMessage)));
        } else {
            _errorBannerLabel->setText(
                QString("Unlink failed (%1): %2")
                    .arg(res.httpStatus)
                    .arg(QString::fromStdString(res.errorMessage)));
        }
        _errorBannerLabel->setVisible(true);
    }
    return res;
}

QString AccountLinkDialog::errorBannerText() const
{
    return _errorBannerLabel ? _errorBannerLabel->text() : QString();
}

QString AccountLinkDialog::conflictBannerText() const
{
    return _conflictBannerLabel ? _conflictBannerLabel->text() : QString();
}

void AccountLinkDialog::setOnAccountSwitchedCallback(
    std::function<void(const PlayerProfileInfo &)> cb)
{
    _onAccountSwitchedCallback = std::move(cb);
}

/* ==================================================================== */
/* 3. CreateTableDialog & PasscodeDialog                                */
/* ==================================================================== */

std::string CreateTableDialog::normalizeObserverPolicy(const std::string &policy)
{
    std::string u = toUpperCopy(policy);
    if (u == "PASSCODE" || u == "SILENT" || u == "SILENT_OBSERVERS") {
        return "SILENT_OBSERVERS";
    }
    if (u == "PRIVATE" || u == "NO_OBSERVERS") {
        return "NO_OBSERVERS";
    }
    return "OPEN";
}

std::string CreateTableDialog::normalizeVoicePolicy(const std::string &policy)
{
    std::string u = toUpperCopy(policy);
    if (u == "ALL_PARTICIPANTS" || u == "ALL_ALLOWED" || u == "ALL") {
        return "ALL_ALLOWED";
    }
    if (u == "DISABLED" || u == "OFF") {
        return "DISABLED";
    }
    return "PLAYERS_ONLY";
}

CreateTableDialog::CreateTableDialog(OctiNetworkClient *netClient,
                                     ThemeId theme,
                                     QWidget *parent)
    : QDialog(parent),
      _netClient(netClient),
      _theme(theme),
      _modeCombo(nullptr),
      _basesCombo(nullptr),
      _superProngsCheck(nullptr),
      _edgelessCheck(nullptr),
      _ratedCheck(nullptr),
      _takebackCheck(nullptr),
      _timeControlCheck(nullptr),
      _initialSecSpin(nullptr),
      _incrementSecSpin(nullptr),
      _observerPolicyCombo(nullptr),
      _voicePolicyCombo(nullptr),
      _passcodeEdit(nullptr),
      _errorLabel(nullptr),
      _createBtn(nullptr)
{
    setWindowTitle("Create OCTI Table");
    setMinimumSize(460, 480);

    QVBoxLayout *root = new QVBoxLayout(this);
    QFormLayout *form = new QFormLayout();

    _modeCombo = new QComboBox(this);
    _modeCombo->addItem("2-Player Standard (9x9)", "2P");
    _modeCombo->addItem("4-Player Partnership (9x9)", "4P");
    _modeCombo->addItem("OCTI for Kids (6x7)", "OFK");

    _basesCombo = new QComboBox(this);
    _basesCombo->addItem("3 Bases Victory", 3);
    _basesCombo->addItem("1 Base Victory", 1);

    _superProngsCheck = new QCheckBox("Enable Superprongs (!)", this);
    _edgelessCheck = new QCheckBox("Enable Edgeless Torus Wrap-Around", this);
    _ratedCheck = new QCheckBox("Rated Match (Elo)", this);
    _ratedCheck->setChecked(true);
    _takebackCheck = new QCheckBox("Allow Undo / Takeback Requests", this);

    _timeControlCheck = new QCheckBox("Enable Fischer Clock", this);
    _initialSecSpin = new QSpinBox(this);
    _initialSecSpin->setRange(10, 14400);
    _initialSecSpin->setValue(600);
    _incrementSecSpin = new QSpinBox(this);
    _incrementSecSpin->setRange(0, 300);
    _incrementSecSpin->setValue(5);

    _observerPolicyCombo = new QComboBox(this);
    _observerPolicyCombo->addItem("Open Observers", "OPEN");
    _observerPolicyCombo->addItem("Silent Observers", "SILENT_OBSERVERS");
    _observerPolicyCombo->addItem("No Observers (Private)", "NO_OBSERVERS");

    _voicePolicyCombo = new QComboBox(this);
    _voicePolicyCombo->addItem("Players Only", "PLAYERS_ONLY");
    _voicePolicyCombo->addItem("All Participants", "ALL_ALLOWED");
    _voicePolicyCombo->addItem("Voice Disabled", "DISABLED");

    _passcodeEdit = new QLineEdit(this);
    _passcodeEdit->setPlaceholderText("Leave empty for public table...");

    form->addRow("Game Mode:", _modeCombo);
    form->addRow("Victory Condition:", _basesCombo);
    form->addRow("", _superProngsCheck);
    form->addRow("", _edgelessCheck);
    form->addRow("", _ratedCheck);
    form->addRow("", _takebackCheck);
    form->addRow("", _timeControlCheck);
    form->addRow("Initial Time (s):", _initialSecSpin);
    form->addRow("Increment (s):", _incrementSecSpin);
    form->addRow("Observer Policy:", _observerPolicyCombo);
    form->addRow("Voice Policy:", _voicePolicyCombo);
    form->addRow("Passcode:", _passcodeEdit);
    root->addLayout(form);

    _errorLabel = new QLabel("", this);
    _errorLabel->setVisible(false);
    root->addWidget(_errorLabel);

    QHBoxLayout *btns = new QHBoxLayout();
    _createBtn = new QPushButton("Create Table", this);
    QPushButton *cancelBtn = new QPushButton("Cancel", this);
    btns->addWidget(_createBtn);
    btns->addWidget(cancelBtn);
    root->addLayout(btns);

    QObject::connect(_createBtn, &QPushButton::clicked, [this]() {
        if (_netClient) {
            submitCreateTable();
        } else {
            accept();
        }
    });
    QObject::connect(cancelBtn, &QPushButton::clicked, [this]() {
        reject();
    });

    setTheme(_theme);
}

CreateTableDialog::~CreateTableDialog() = default;

void CreateTableDialog::setTheme(ThemeId theme)
{
    _theme = theme;
    setStyleSheet(buildAppStyleSheet(_theme));
}

ThemeId CreateTableDialog::theme() const
{
    return _theme;
}

void CreateTableDialog::setRequest(const CreateTableRequest &req)
{
    int mIdx = _modeCombo->findData(QString::fromStdString(req.mode));
    if (mIdx >= 0) {
        _modeCombo->setCurrentIndex(mIdx);
    }
    int bIdx = _basesCombo->findData(req.bases);
    if (bIdx >= 0) {
        _basesCombo->setCurrentIndex(bIdx);
    }
    _superProngsCheck->setChecked(req.superProngs);
    _edgelessCheck->setChecked(req.edgeless);
    _ratedCheck->setChecked(req.rated);
    _takebackCheck->setChecked(req.allowTakeback);
    _timeControlCheck->setChecked(req.timeControlEnabled);
    if (req.initialMs > 0) {
        _initialSecSpin->setValue(static_cast<int>(req.initialMs / 1000));
    }
    _incrementSecSpin->setValue(static_cast<int>(req.incrementMs / 1000));

    std::string normObs = normalizeObserverPolicy(req.observerPolicy);
    int oIdx = _observerPolicyCombo->findData(QString::fromStdString(normObs));
    if (oIdx >= 0) {
        _observerPolicyCombo->setCurrentIndex(oIdx);
    }
    std::string normVoice = normalizeVoicePolicy(req.voicePolicy);
    int vIdx = _voicePolicyCombo->findData(QString::fromStdString(normVoice));
    if (vIdx >= 0) {
        _voicePolicyCombo->setCurrentIndex(vIdx);
    }
    _passcodeEdit->setText(QString::fromStdString(req.passcode));
}

CreateTableRequest CreateTableDialog::buildRequest() const
{
    CreateTableRequest req;
    req.mode = _modeCombo->currentData().toString().toStdString();
    req.bases = _basesCombo->currentData().toInt();
    req.superProngs = _superProngsCheck->isChecked();
    req.edgeless = _edgelessCheck->isChecked();
    req.rated = _ratedCheck->isChecked();
    req.allowTakeback = _takebackCheck->isChecked();
    req.timeControlEnabled = _timeControlCheck->isChecked();
    req.initialMs =
        static_cast<std::int64_t>(_initialSecSpin->value()) * 1000LL;
    req.incrementMs =
        static_cast<std::int64_t>(_incrementSecSpin->value()) * 1000LL;
    req.observerPolicy = normalizeObserverPolicy(
        _observerPolicyCombo->currentData().toString().toStdString());
    req.voicePolicy = normalizeVoicePolicy(
        _voicePolicyCombo->currentData().toString().toStdString());
    req.passcode = _passcodeEdit->text().trimmed().toStdString();
    return req;
}

ApiResult<CreateTableResponse> CreateTableDialog::submitCreateTable()
{
    ApiResult<CreateTableResponse> res;
    if (!_netClient) {
        res.errorMessage = "Network client is null";
        return res;
    }
    res = _netClient->createTable(buildRequest());
    if (res.ok) {
        _createdTable = res.data;
        if (_errorLabel) {
            _errorLabel->clear();
            _errorLabel->setVisible(false);
        }
        accept();
    } else if (_errorLabel) {
        _errorLabel->setText(
            QString("Create table failed (%1): %2")
                .arg(res.httpStatus)
                .arg(QString::fromStdString(res.errorMessage)));
        _errorLabel->setVisible(true);
    }
    return res;
}

const CreateTableResponse &CreateTableDialog::createdTable() const
{
    return _createdTable;
}

QString CreateTableDialog::errorText() const
{
    return _errorLabel ? _errorLabel->text() : QString();
}

PasscodeDialog::PasscodeDialog(const std::string &roomId,
                               ThemeId theme,
                               QWidget *parent)
    : QDialog(parent),
      _theme(theme),
      _passcodeEdit(nullptr)
{
    setWindowTitle("Enter Table Passcode");
    setMinimumSize(340, 160);

    QVBoxLayout *root = new QVBoxLayout(this);
    QLabel *lbl = new QLabel(
        QString("Table %1 requires a passcode:")
            .arg(QString::fromStdString(roomId)),
        this);
    root->addWidget(lbl);

    _passcodeEdit = new QLineEdit(this);
    _passcodeEdit->setEchoMode(QLineEdit::Password);
    _passcodeEdit->setPlaceholderText("Passcode...");
    root->addWidget(_passcodeEdit);

    QHBoxLayout *btns = new QHBoxLayout();
    QPushButton *okBtn = new QPushButton("Join / Observe", this);
    QPushButton *cancelBtn = new QPushButton("Cancel", this);
    btns->addWidget(okBtn);
    btns->addWidget(cancelBtn);
    root->addLayout(btns);

    QObject::connect(okBtn, &QPushButton::clicked, [this]() {
        accept();
    });
    QObject::connect(cancelBtn, &QPushButton::clicked, [this]() {
        reject();
    });

    setStyleSheet(buildAppStyleSheet(_theme));
}

PasscodeDialog::~PasscodeDialog() = default;

void PasscodeDialog::setPasscode(const std::string &code)
{
    if (_passcodeEdit) {
        _passcodeEdit->setText(QString::fromStdString(code));
    }
}

std::string PasscodeDialog::passcode() const
{
    return _passcodeEdit ? _passcodeEdit->text().toStdString() : std::string();
}

/* ==================================================================== */
/* 4. MatchmakingDialog                                                 */
/* ==================================================================== */

MatchmakingDialog::MatchmakingDialog(OctiNetworkClient *netClient,
                                     ThemeId theme,
                                     QWidget *parent)
    : QDialog(parent),
      _netClient(netClient),
      _theme(theme),
      _modeCombo(nullptr),
      _basesCombo(nullptr),
      _superProngsCheck(nullptr),
      _edgelessCheck(nullptr),
      _timeControlCheck(nullptr),
      _initialSecSpin(nullptr),
      _incrementSecSpin(nullptr),
      _statusLabel(nullptr),
      _startBtn(nullptr),
      _cancelBtn(nullptr)
{
    setWindowTitle("OCTI Elo Matchmaking");
    setMinimumSize(420, 340);

    QVBoxLayout *root = new QVBoxLayout(this);
    QFormLayout *form = new QFormLayout();

    _modeCombo = new QComboBox(this);
    _modeCombo->addItem("2-Player Standard", "2P");
    _modeCombo->addItem("4-Player Partnership", "4P");
    _modeCombo->addItem("OCTI for Kids", "OFK");

    _basesCombo = new QComboBox(this);
    _basesCombo->addItem("3 Bases", 3);
    _basesCombo->addItem("1 Base", 1);

    _superProngsCheck = new QCheckBox("Superprongs (!)", this);
    _edgelessCheck = new QCheckBox("Edgeless Board", this);
    _timeControlCheck = new QCheckBox("Fischer Clock", this);
    _initialSecSpin = new QSpinBox(this);
    _initialSecSpin->setRange(10, 14400);
    _initialSecSpin->setValue(600);
    _incrementSecSpin = new QSpinBox(this);
    _incrementSecSpin->setRange(0, 300);
    _incrementSecSpin->setValue(5);

    form->addRow("Mode:", _modeCombo);
    form->addRow("Bases:", _basesCombo);
    form->addRow("", _superProngsCheck);
    form->addRow("", _edgelessCheck);
    form->addRow("", _timeControlCheck);
    form->addRow("Initial (s):", _initialSecSpin);
    form->addRow("Increment (s):", _incrementSecSpin);
    root->addLayout(form);

    _statusLabel = new QLabel("Idle — select preferences and click Find Match.", this);
    _statusLabel->setWordWrap(true);
    root->addWidget(_statusLabel);

    QHBoxLayout *btns = new QHBoxLayout();
    _startBtn = new QPushButton("Find Match", this);
    _cancelBtn = new QPushButton("Cancel Queue", this);
    btns->addWidget(_startBtn);
    btns->addWidget(_cancelBtn);
    root->addLayout(btns);

    QObject::connect(_startBtn, &QPushButton::clicked, [this]() {
        startMatchmaking();
    });
    QObject::connect(_cancelBtn, &QPushButton::clicked, [this]() {
        cancelMatchmaking();
        reject();
    });
    QObject::connect(&_pollTimer, &QTimer::timeout, [this]() {
        pollMatchmakingStatus();
    });

    setTheme(_theme);
}

MatchmakingDialog::~MatchmakingDialog()
{
    _pollTimer.stop();
}

void MatchmakingDialog::setTheme(ThemeId theme)
{
    _theme = theme;
    setStyleSheet(buildAppStyleSheet(_theme));
}

ThemeId MatchmakingDialog::theme() const
{
    return _theme;
}

void MatchmakingDialog::setRequest(const MatchmakingRequest &req)
{
    int mIdx = _modeCombo->findData(QString::fromStdString(req.mode));
    if (mIdx >= 0) {
        _modeCombo->setCurrentIndex(mIdx);
    }
    int bIdx = _basesCombo->findData(req.bases);
    if (bIdx >= 0) {
        _basesCombo->setCurrentIndex(bIdx);
    }
    _superProngsCheck->setChecked(req.superProngs);
    _edgelessCheck->setChecked(req.edgeless);
    _timeControlCheck->setChecked(req.timeControlEnabled);
    if (req.initialMs > 0) {
        _initialSecSpin->setValue(static_cast<int>(req.initialMs / 1000));
    }
    _incrementSecSpin->setValue(static_cast<int>(req.incrementMs / 1000));
}

MatchmakingRequest MatchmakingDialog::buildRequest() const
{
    MatchmakingRequest req;
    req.mode = _modeCombo->currentData().toString().toStdString();
    req.bases = _basesCombo->currentData().toInt();
    req.superProngs = _superProngsCheck->isChecked();
    req.edgeless = _edgelessCheck->isChecked();
    req.timeControlEnabled = _timeControlCheck->isChecked();
    req.initialMs =
        static_cast<std::int64_t>(_initialSecSpin->value()) * 1000LL;
    req.incrementMs =
        static_cast<std::int64_t>(_incrementSecSpin->value()) * 1000LL;
    return req;
}

ApiResult<bool> MatchmakingDialog::startMatchmaking()
{
    ApiResult<bool> res;
    if (!_netClient) {
        res.errorMessage = "Network client is null";
        return res;
    }
    res = _netClient->enqueueMatchmaking(buildRequest());
    if (res.ok) {
        _lastStatus.queued = true;
        if (_statusLabel) {
            _statusLabel->setText("Searching for opponent...");
        }
        _pollTimer.start(250);
    } else if (_statusLabel) {
        _statusLabel->setText(
            QString("Queue failed: %1")
                .arg(QString::fromStdString(res.errorMessage)));
    }
    return res;
}

ApiResult<MatchmakingStatusData> MatchmakingDialog::pollMatchmakingStatus()
{
    ApiResult<MatchmakingStatusData> res;
    if (!_netClient) {
        res.errorMessage = "Network client is null";
        return res;
    }
    res = _netClient->fetchMatchmakingStatus();
    if (res.ok) {
        _lastStatus = res.data;
        if (!_lastStatus.matchedRoomId.empty()) {
            _pollTimer.stop();
            if (_statusLabel) {
                _statusLabel->setText(
                    QString("Match found! Room %1")
                        .arg(QString::fromStdString(_lastStatus.matchedRoomId)));
            }
            if (_onMatchedCallback) {
                _onMatchedCallback(_lastStatus.matchedRoomId);
            }
            accept();
        } else if (_lastStatus.queued && _statusLabel) {
            _statusLabel->setText(
                QString("Queued (%1 ms) • Rating %2 • Window ±%3")
                    .arg(_lastStatus.waitedMs)
                    .arg(_lastStatus.rating)
                    .arg(_lastStatus.window));
        }
    }
    return res;
}

ApiResult<bool> MatchmakingDialog::cancelMatchmaking()
{
    _pollTimer.stop();
    ApiResult<bool> res;
    if (!_netClient) {
        res.errorMessage = "Network client is null";
        return res;
    }
    res = _netClient->dequeueMatchmaking();
    _lastStatus.queued = false;
    return res;
}

const MatchmakingStatusData &MatchmakingDialog::lastStatus() const
{
    return _lastStatus;
}

QString MatchmakingDialog::statusText() const
{
    return _statusLabel ? _statusLabel->text() : QString();
}

void MatchmakingDialog::setOnMatchedCallback(
    std::function<void(const std::string &roomId)> cb)
{
    _onMatchedCallback = std::move(cb);
}

/* ==================================================================== */
/* 5. RoomModerationDialog                                              */
/* ==================================================================== */

RoomModerationDialog::RoomModerationDialog(OctiNetworkClient *netClient,
                                           const std::string &roomId,
                                           ThemeId theme,
                                           QWidget *parent)
    : QDialog(parent),
      _netClient(netClient),
      _roomId(roomId),
      _theme(theme),
      _observerPolicyCombo(nullptr),
      _voicePolicyCombo(nullptr),
      _applyPoliciesBtn(nullptr),
      _participantsTable(nullptr),
      _targetUuidEdit(nullptr),
      _banReasonEdit(nullptr),
      _muteChatCheck(nullptr),
      _muteVoiceCheck(nullptr),
      _muteBtn(nullptr),
      _banBtn(nullptr),
      _kickSeatBtn(nullptr),
      _statusLabel(nullptr)
{
    setWindowTitle("Room Moderation & Policies");
    setMinimumSize(560, 480);

    QVBoxLayout *root = new QVBoxLayout(this);

    QGroupBox *policyBox = new QGroupBox("Live Room Policies", this);
    QFormLayout *pForm = new QFormLayout(policyBox);
    _observerPolicyCombo = new QComboBox(policyBox);
    _observerPolicyCombo->addItem("OPEN", "OPEN");
    _observerPolicyCombo->addItem("SILENT_OBSERVERS", "SILENT_OBSERVERS");
    _observerPolicyCombo->addItem("NO_OBSERVERS", "NO_OBSERVERS");

    _voicePolicyCombo = new QComboBox(policyBox);
    _voicePolicyCombo->addItem("PLAYERS_ONLY", "PLAYERS_ONLY");
    _voicePolicyCombo->addItem("ALL_ALLOWED", "ALL_ALLOWED");
    _voicePolicyCombo->addItem("DISABLED", "DISABLED");

    _applyPoliciesBtn = new QPushButton("Apply Policies", policyBox);
    QObject::connect(_applyPoliciesBtn, &QPushButton::clicked, [this]() {
        applyPolicySettings(
            _observerPolicyCombo->currentData().toString().toStdString(),
            _voicePolicyCombo->currentData().toString().toStdString());
    });

    pForm->addRow("Observer Policy:", _observerPolicyCombo);
    pForm->addRow("Voice Policy:", _voicePolicyCombo);
    pForm->addRow("", _applyPoliciesBtn);
    root->addWidget(policyBox);

    _participantsTable = new QTableWidget(0, 3, this);
    _participantsTable->setHorizontalHeaderLabels(
        QStringList() << "Role / Seat" << "Handle" << "Player UUID");
    _participantsTable->horizontalHeader()->setSectionResizeMode(
        QHeaderView::Stretch);
    _participantsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    _participantsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    QObject::connect(_participantsTable, &QTableWidget::cellClicked,
                     [this](int row, int /*col*/) {
        QTableWidgetItem *uuidItem = _participantsTable->item(row, 2);
        if (uuidItem && _targetUuidEdit) {
            _targetUuidEdit->setText(uuidItem->text());
        }
    });
    root->addWidget(_participantsTable);

    QGroupBox *actBox = new QGroupBox("Participant Moderation", this);
    QFormLayout *aForm = new QFormLayout(actBox);
    _targetUuidEdit = new QLineEdit(actBox);
    _targetUuidEdit->setPlaceholderText("Target Player UUID...");
    _banReasonEdit = new QLineEdit(actBox);
    _banReasonEdit->setPlaceholderText("Reason for kick/ban...");
    _muteChatCheck = new QCheckBox("Mute Chat", actBox);
    _muteVoiceCheck = new QCheckBox("Mute Voice", actBox);

    QHBoxLayout *actBtns = new QHBoxLayout();
    _muteBtn = new QPushButton("Update Mute", actBox);
    _banBtn = new QPushButton("Kick & Ban Observer", actBox);
    _kickSeatBtn = new QPushButton("Kick Pre-Game Seat", actBox);
    actBtns->addWidget(_muteBtn);
    actBtns->addWidget(_banBtn);
    actBtns->addWidget(_kickSeatBtn);

    QObject::connect(_muteBtn, &QPushButton::clicked, [this]() {
        muteParticipant(
            _targetUuidEdit->text().trimmed().toStdString(),
            _muteChatCheck->isChecked(),
            _muteVoiceCheck->isChecked());
    });
    QObject::connect(_banBtn, &QPushButton::clicked, [this]() {
        kickAndBanObserver(
            _targetUuidEdit->text().trimmed().toStdString(),
            _banReasonEdit->text().trimmed().toStdString());
    });
    QObject::connect(_kickSeatBtn, &QPushButton::clicked, [this]() {
        kickWaitingSeatByUuid(
            _targetUuidEdit->text().trimmed().toStdString());
    });

    aForm->addRow("Target UUID:", _targetUuidEdit);
    aForm->addRow("Ban Reason:", _banReasonEdit);
    aForm->addRow("", _muteChatCheck);
    aForm->addRow("", _muteVoiceCheck);
    aForm->addRow("", actBtns);
    root->addWidget(actBox);

    _statusLabel = new QLabel("", this);
    root->addWidget(_statusLabel);

    QPushButton *closeBtn = new QPushButton("Close", this);
    QObject::connect(closeBtn, &QPushButton::clicked, [this]() {
        accept();
    });
    root->addWidget(closeBtn);

    setTheme(_theme);
    refreshRoomView();
}

RoomModerationDialog::~RoomModerationDialog() = default;

void RoomModerationDialog::setTheme(ThemeId theme)
{
    _theme = theme;
    setStyleSheet(buildAppStyleSheet(_theme));
}

ThemeId RoomModerationDialog::theme() const
{
    return _theme;
}

bool RoomModerationDialog::refreshRoomView()
{
    if (!_netClient || _roomId.empty()) {
        return false;
    }
    auto res = _netClient->fetchGameView(_roomId);
    if (!res.ok) {
        return false;
    }
    populateFromGameView(res.data);
    return true;
}

void RoomModerationDialog::populateFromGameView(const GameViewData &view)
{
    _gameView = view;
    int oIdx = _observerPolicyCombo->findData(
        QString::fromStdString(view.observerPolicy));
    if (oIdx >= 0) {
        _observerPolicyCombo->setCurrentIndex(oIdx);
    }
    int vIdx = _voicePolicyCombo->findData(
        QString::fromStdString(view.voicePolicy));
    if (vIdx >= 0) {
        _voicePolicyCombo->setCurrentIndex(vIdx);
    }

    int totalRows = static_cast<int>(view.seats.size() + view.observers.size());
    _participantsTable->setRowCount(totalRows);
    int r = 0;
    for (std::size_t i = 0; i < view.seats.size(); ++i, ++r) {
        const GameSeatInfo &s = view.seats[i];
        _participantsTable->setItem(
            r, 0, new QTableWidgetItem(QString("Seat %1").arg(s.seat)));
        _participantsTable->setItem(
            r, 1, new QTableWidgetItem(QString::fromStdString(s.handle)));
        _participantsTable->setItem(
            r, 2, new QTableWidgetItem(QString::fromStdString(s.playerUuid)));
    }
    for (std::size_t i = 0; i < view.observers.size(); ++i, ++r) {
        _participantsTable->setItem(
            r, 0, new QTableWidgetItem("Observer"));
        _participantsTable->setItem(
            r, 1, new QTableWidgetItem("Observer"));
        _participantsTable->setItem(
            r, 2, new QTableWidgetItem(QString::fromStdString(view.observers[i])));
    }
}

const GameViewData &RoomModerationDialog::currentGameView() const
{
    return _gameView;
}

ApiResult<RoomSettingsData> RoomModerationDialog::applyPolicySettings(
    const std::string &observerPolicy,
    const std::string &voicePolicy)
{
    ApiResult<RoomSettingsData> res;
    if (!_netClient) {
        res.errorMessage = "Network client is null";
        return res;
    }
    const std::string normObs =
        CreateTableDialog::normalizeObserverPolicy(observerPolicy);
    const std::string normVoice =
        CreateTableDialog::normalizeVoicePolicy(voicePolicy);
    res = _netClient->updateRoomSettings(_roomId, normObs, normVoice);
    if (res.ok) {
        _gameView.observerPolicy = res.data.observerPolicy;
        _gameView.voicePolicy = res.data.voicePolicy;
        if (_statusLabel) {
            _statusLabel->setText("Room policies updated.");
        }
    } else if (_statusLabel) {
        _statusLabel->setText(
            QString("Policy update failed: %1")
                .arg(QString::fromStdString(res.errorMessage)));
    }
    return res;
}

ApiResult<bool> RoomModerationDialog::muteParticipant(
    const std::string &targetUuid,
    bool muteChat,
    bool muteVoice)
{
    ApiResult<bool> res;
    if (!_netClient) {
        res.errorMessage = "Network client is null";
        return res;
    }
    res = _netClient->muteParticipant(_roomId, targetUuid, muteChat, muteVoice);
    if (_statusLabel) {
        _statusLabel->setText(
            res.ok ? "Mute state updated."
                   : QString("Mute failed: %1")
                         .arg(QString::fromStdString(res.errorMessage)));
    }
    return res;
}

ApiResult<bool> RoomModerationDialog::kickAndBanObserver(
    const std::string &targetUuid,
    const std::string &reason)
{
    ApiResult<bool> res;
    if (!_netClient) {
        res.errorMessage = "Network client is null";
        return res;
    }
    res = _netClient->banObserver(_roomId, targetUuid, reason);
    if (res.ok) {
        refreshRoomView();
    }
    if (_statusLabel) {
        _statusLabel->setText(
            res.ok ? "Observer banned."
                   : QString("Ban failed: %1")
                         .arg(QString::fromStdString(res.errorMessage)));
    }
    return res;
}

ApiResult<bool> RoomModerationDialog::kickWaitingSeat(int seat)
{
    ApiResult<bool> res;
    if (!_netClient) {
        res.errorMessage = "Network client is null";
        return res;
    }
    res = _netClient->kickSeat(_roomId, seat);
    if (res.ok) {
        refreshRoomView();
    }
    return res;
}

ApiResult<bool> RoomModerationDialog::kickWaitingSeatByUuid(
    const std::string &targetUuid)
{
    ApiResult<bool> res;
    if (!_netClient) {
        res.errorMessage = "Network client is null";
        return res;
    }
    res = _netClient->kickSeat(_roomId, targetUuid);
    if (res.ok) {
        refreshRoomView();
    }
    return res;
}

QString RoomModerationDialog::statusText() const
{
    return _statusLabel ? _statusLabel->text() : QString();
}

/* ==================================================================== */
/* 6. ArchiveBrowserDialog & LeaderboardDialog                          */
/* ==================================================================== */

ArchiveBrowserDialog::ArchiveBrowserDialog(OctiNetworkClient *netClient,
                                           ThemeId theme,
                                           QWidget *parent)
    : QDialog(parent),
      _netClient(netClient),
      _theme(theme),
      _modeFilterCombo(nullptr),
      _playerFilterEdit(nullptr),
      _refreshBtn(nullptr),
      _gamesTable(nullptr),
      _transcriptPreview(nullptr),
      _downloadBtn(nullptr),
      _loadReplayBtn(nullptr),
      _statusLabel(nullptr)
{
    setWindowTitle("OCTI Game Archive & Replay Browser");
    setMinimumSize(680, 520);

    QVBoxLayout *root = new QVBoxLayout(this);

    QHBoxLayout *filterBar = new QHBoxLayout();
    _modeFilterCombo = new QComboBox(this);
    _modeFilterCombo->addItem("All Modes", "");
    _modeFilterCombo->addItem("2P", "2P");
    _modeFilterCombo->addItem("4P", "4P");
    _modeFilterCombo->addItem("OFK", "OFK");

    _playerFilterEdit = new QLineEdit(this);
    _playerFilterEdit->setPlaceholderText("Filter by Player UUID...");
    _refreshBtn = new QPushButton("Search Archive", this);

    filterBar->addWidget(new QLabel("Mode:", this));
    filterBar->addWidget(_modeFilterCombo);
    filterBar->addWidget(_playerFilterEdit);
    filterBar->addWidget(_refreshBtn);
    root->addLayout(filterBar);

    _gamesTable = new QTableWidget(0, 5, this);
    _gamesTable->setHorizontalHeaderLabels(
        QStringList() << "Game ID" << "Mode" << "Players" << "Result" << "Plies");
    _gamesTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    _gamesTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    _gamesTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    root->addWidget(_gamesTable);

    _transcriptPreview = new QPlainTextEdit(this);
    _transcriptPreview->setReadOnly(true);
    _transcriptPreview->setMaximumHeight(140);
    root->addWidget(_transcriptPreview);

    QHBoxLayout *actBar = new QHBoxLayout();
    _downloadBtn = new QPushButton("Fetch .octi Transcript", this);
    _loadReplayBtn = new QPushButton("Load into Replay Board", this);
    QPushButton *closeBtn = new QPushButton("Close", this);
    actBar->addWidget(_downloadBtn);
    actBar->addWidget(_loadReplayBtn);
    actBar->addWidget(closeBtn);
    root->addLayout(actBar);

    _statusLabel = new QLabel("", this);
    root->addWidget(_statusLabel);

    QObject::connect(_refreshBtn, &QPushButton::clicked, [this]() {
        refreshArchiveList(
            _modeFilterCombo->currentData().toString().toStdString(),
            _playerFilterEdit->text().trimmed().toStdString());
    });
    QObject::connect(_gamesTable, &QTableWidget::cellClicked,
                     [this](int row, int /*col*/) {
        if (row >= 0 && row < static_cast<int>(_games.size())) {
            const std::string gid = _games[static_cast<std::size_t>(row)].gameId;
            inspectArchivedGame(gid);
            fetchTranscriptForGame(gid);
        }
    });
    QObject::connect(_downloadBtn, &QPushButton::clicked, [this]() {
        int row = _gamesTable->currentRow();
        if (row >= 0 && row < static_cast<int>(_games.size())) {
            fetchTranscriptForGame(
                _games[static_cast<std::size_t>(row)].gameId);
        }
    });
    QObject::connect(_loadReplayBtn, &QPushButton::clicked, [this]() {
        int row = _gamesTable->currentRow();
        if (row >= 0 && row < static_cast<int>(_games.size())) {
            if (loadGameIntoReplay(
                    _games[static_cast<std::size_t>(row)].gameId)) {
                accept();
            }
        }
    });
    QObject::connect(closeBtn, &QPushButton::clicked, [this]() {
        reject();
    });

    setTheme(_theme);
}

ArchiveBrowserDialog::~ArchiveBrowserDialog() = default;

void ArchiveBrowserDialog::setTheme(ThemeId theme)
{
    _theme = theme;
    setStyleSheet(buildAppStyleSheet(_theme));
}

ThemeId ArchiveBrowserDialog::theme() const
{
    return _theme;
}

ApiResult<ArchivedGamesPage> ArchiveBrowserDialog::refreshArchiveList(
    const std::string &mode,
    const std::string &playerUuid,
    int limit,
    int offset)
{
    ApiResult<ArchivedGamesPage> res;
    if (!_netClient) {
        res.errorMessage = "Network client is null";
        return res;
    }
    res = _netClient->listArchivedGames(mode, playerUuid, limit, offset);
    if (res.ok) {
        _games = res.data.games;
        rebuildGamesTable();
        if (_statusLabel) {
            _statusLabel->setText(
                QString("Loaded %1 archived game(s).")
                    .arg(static_cast<int>(_games.size())));
        }
    }
    return res;
}

void ArchiveBrowserDialog::rebuildGamesTable()
{
    if (!_gamesTable) {
        return;
    }
    _gamesTable->setRowCount(static_cast<int>(_games.size()));
    for (int i = 0; i < static_cast<int>(_games.size()); ++i) {
        const ArchivedGameSummary &g = _games[static_cast<std::size_t>(i)];
        QString playersStr;
        for (std::size_t k = 0; k < g.players.size(); ++k) {
            if (k > 0) {
                playersStr += " vs ";
            }
            playersStr += QString::fromStdString(g.players[k].handle);
        }
        _gamesTable->setItem(
            i, 0, new QTableWidgetItem(QString::fromStdString(g.gameId)));
        _gamesTable->setItem(
            i, 1, new QTableWidgetItem(QString::fromStdString(g.mode)));
        _gamesTable->setItem(
            i, 2, new QTableWidgetItem(playersStr));
        _gamesTable->setItem(
            i, 3, new QTableWidgetItem(QString::fromStdString(g.result)));
        _gamesTable->setItem(
            i, 4, new QTableWidgetItem(QString::number(g.plyCount)));
    }
}

ApiResult<ArchivedGameSummary> ArchiveBrowserDialog::inspectArchivedGame(
    const std::string &gameId)
{
    ApiResult<ArchivedGameSummary> res;
    if (!_netClient) {
        res.errorMessage = "Network client is null";
        return res;
    }
    res = _netClient->fetchArchivedGame(gameId);
    if (res.ok) {
        _inspectedGame = res.data;
    }
    return res;
}

ApiResult<std::string> ArchiveBrowserDialog::fetchTranscriptForGame(
    const std::string &gameId)
{
    ApiResult<std::string> res;
    if (!_netClient) {
        res.errorMessage = "Network client is null";
        return res;
    }
    res = _netClient->downloadOctiTranscript(gameId);
    if (res.ok) {
        _lastTranscript = res.data;
        if (_transcriptPreview) {
            _transcriptPreview->setPlainText(
                QString::fromStdString(_lastTranscript));
        }
    }
    return res;
}

bool ArchiveBrowserDialog::loadGameIntoReplay(const std::string &gameId)
{
    if (!_netClient) {
        return false;
    }
    auto sumRes = inspectArchivedGame(gameId);
    auto trRes = fetchTranscriptForGame(gameId);
    if (!trRes.ok || trRes.data.empty()) {
        return false;
    }
    if (_onLoadReplayCallback) {
        _onLoadReplayCallback(gameId, trRes.data, sumRes.data);
    }
    return true;
}

const std::vector<ArchivedGameSummary> &ArchiveBrowserDialog::archivedGames() const
{
    return _games;
}

const ArchivedGameSummary &ArchiveBrowserDialog::inspectedGame() const
{
    return _inspectedGame;
}

const std::string &ArchiveBrowserDialog::lastDownloadedTranscript() const
{
    return _lastTranscript;
}

void ArchiveBrowserDialog::setOnLoadReplayCallback(
    std::function<void(const std::string &gameId,
                       const std::string &octiTranscript,
                       const ArchivedGameSummary &summary)> cb)
{
    _onLoadReplayCallback = std::move(cb);
}

LeaderboardDialog::LeaderboardDialog(OctiNetworkClient *netClient,
                                     ThemeId theme,
                                     QWidget *parent)
    : QDialog(parent),
      _netClient(netClient),
      _theme(theme),
      _modeCombo(nullptr),
      _refreshBtn(nullptr),
      _table(nullptr)
{
    setWindowTitle("OCTI Global Elo Leaderboard");
    setMinimumSize(600, 440);

    QVBoxLayout *root = new QVBoxLayout(this);

    QHBoxLayout *topBar = new QHBoxLayout();
    _modeCombo = new QComboBox(this);
    _modeCombo->addItem("2-Player Standard", "2P");
    _modeCombo->addItem("4-Player Partnership", "4P");
    _modeCombo->addItem("OCTI for Kids", "OFK");
    _refreshBtn = new QPushButton("Refresh", this);
    topBar->addWidget(new QLabel("Mode:", this));
    topBar->addWidget(_modeCombo);
    topBar->addWidget(_refreshBtn);
    root->addLayout(topBar);

    _table = new QTableWidget(0, 7, this);
    _table->setHorizontalHeaderLabels(
        QStringList() << "Rank" << "Handle" << "Elo" << "Games"
                      << "Wins" << "Losses" << "Draws");
    _table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    _table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    root->addWidget(_table);

    QPushButton *closeBtn = new QPushButton("Close", this);
    QObject::connect(closeBtn, &QPushButton::clicked, [this]() {
        accept();
    });
    root->addWidget(closeBtn);

    QObject::connect(
        _modeCombo,
        static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
        [this](int /*idx*/) {
            refreshLeaderboard(selectedMode());
        });
    QObject::connect(_refreshBtn, &QPushButton::clicked, [this]() {
        refreshLeaderboard(selectedMode());
    });

    setTheme(_theme);
}

LeaderboardDialog::~LeaderboardDialog() = default;

void LeaderboardDialog::setTheme(ThemeId theme)
{
    _theme = theme;
    setStyleSheet(buildAppStyleSheet(_theme));
}

ThemeId LeaderboardDialog::theme() const
{
    return _theme;
}

std::string LeaderboardDialog::selectedMode() const
{
    return _modeCombo ? _modeCombo->currentData().toString().toStdString()
                      : std::string("2P");
}

ApiResult<std::vector<LeaderboardRow>> LeaderboardDialog::refreshLeaderboard(
    const std::string &mode,
    int limit)
{
    ApiResult<std::vector<LeaderboardRow>> res;
    if (!_netClient) {
        res.errorMessage = "Network client is null";
        return res;
    }
    int idx = _modeCombo->findData(QString::fromStdString(mode));
    if (idx >= 0 && _modeCombo->currentIndex() != idx) {
        _modeCombo->blockSignals(true);
        _modeCombo->setCurrentIndex(idx);
        _modeCombo->blockSignals(false);
    }
    res = _netClient->fetchLeaderboard(mode, limit);
    if (res.ok) {
        _rows = res.data;
        rebuildTable();
    }
    return res;
}

const std::vector<LeaderboardRow> &LeaderboardDialog::rows() const
{
    return _rows;
}

void LeaderboardDialog::rebuildTable()
{
    if (!_table) {
        return;
    }
    _table->setRowCount(static_cast<int>(_rows.size()));
    for (int i = 0; i < static_cast<int>(_rows.size()); ++i) {
        const LeaderboardRow &r = _rows[static_cast<std::size_t>(i)];
        _table->setItem(i, 0, new QTableWidgetItem(QString::number(r.rank)));
        _table->setItem(i, 1, new QTableWidgetItem(QString::fromStdString(r.handle)));
        _table->setItem(i, 2, new QTableWidgetItem(QString::number(r.elo)));
        _table->setItem(i, 3, new QTableWidgetItem(QString::number(r.gamesPlayed)));
        _table->setItem(i, 4, new QTableWidgetItem(QString::number(r.wins)));
        _table->setItem(i, 5, new QTableWidgetItem(QString::number(r.losses)));
        _table->setItem(i, 6, new QTableWidgetItem(QString::number(r.draws)));
    }
}

/* ==================================================================== */
/* 7. NewLocalGameDialog & GameOverDialog                               */
/* ==================================================================== */

NewLocalGameDialog::NewLocalGameDialog(ThemeId theme, QWidget *parent)
    : QDialog(parent),
      _theme(theme),
      _playTypeCombo(nullptr),
      _difficultyCombo(nullptr),
      _humanSeatCombo(nullptr),
      _modeCombo(nullptr),
      _basesCombo(nullptr),
      _superProngsCheck(nullptr),
      _edgelessCheck(nullptr),
      _timeControlCheck(nullptr),
      _initialSecSpin(nullptr),
      _incrementSecSpin(nullptr)
{
    setWindowTitle("New Offline / Practice Game");
    setMinimumSize(440, 420);

    QVBoxLayout *root = new QVBoxLayout(this);
    QFormLayout *form = new QFormLayout();

    _playTypeCombo = new QComboBox(this);
    _playTypeCombo->addItem(
        "Practice vs. AI Bot", static_cast<int>(LocalPlayType::VS_AI_BOT));
    _playTypeCombo->addItem(
        "Local Pass-and-Play", static_cast<int>(LocalPlayType::PASS_AND_PLAY));

    _difficultyCombo = new QComboBox(this);
    _difficultyCombo->addItem("Casual", static_cast<int>(BotDifficulty::CASUAL));
    _difficultyCombo->addItem("Tactical", static_cast<int>(BotDifficulty::TACTICAL));
    _difficultyCombo->addItem("Master", static_cast<int>(BotDifficulty::MASTER));
    _difficultyCombo->setCurrentIndex(1);

    _humanSeatCombo = new QComboBox(this);
    _humanSeatCombo->addItem("Seat A (South / Emerald)", static_cast<int>(PLAYER_A));
    _humanSeatCombo->addItem("Seat B (North/West / Crimson)", static_cast<int>(PLAYER_B));
    _humanSeatCombo->addItem("Seat C (North 4P / Sapphire)", static_cast<int>(PLAYER_C));
    _humanSeatCombo->addItem("Seat D (East 4P / Amber)", static_cast<int>(PLAYER_D));

    _modeCombo = new QComboBox(this);
    _modeCombo->addItem("2-Player Standard (9x9)", static_cast<int>(GAME_MODE_2));
    _modeCombo->addItem("4-Player Partnership (9x9)", static_cast<int>(GAME_MODE_4));
    _modeCombo->addItem("OCTI for Kids (6x7)", static_cast<int>(GAME_MODE_OFK));

    _basesCombo = new QComboBox(this);
    _basesCombo->addItem("3 Bases Victory", static_cast<int>(GAME_BASES_3));
    _basesCombo->addItem("1 Base Victory", static_cast<int>(GAME_BASES_1));

    _superProngsCheck = new QCheckBox("Use Superprongs (!)", this);
    _edgelessCheck = new QCheckBox("Use Edgeless Torus Board", this);
    _timeControlCheck = new QCheckBox("Enable Fischer Clock", this);

    _initialSecSpin = new QSpinBox(this);
    _initialSecSpin->setRange(10, 14400);
    _initialSecSpin->setValue(600);
    _incrementSecSpin = new QSpinBox(this);
    _incrementSecSpin->setRange(0, 300);
    _incrementSecSpin->setValue(5);

    form->addRow("Play Mode:", _playTypeCombo);
    form->addRow("Bot Difficulty:", _difficultyCombo);
    form->addRow("Your Seat:", _humanSeatCombo);
    form->addRow("Ruleset:", _modeCombo);
    form->addRow("Victory:", _basesCombo);
    form->addRow("", _superProngsCheck);
    form->addRow("", _edgelessCheck);
    form->addRow("", _timeControlCheck);
    form->addRow("Initial Time (s):", _initialSecSpin);
    form->addRow("Increment (s):", _incrementSecSpin);
    root->addLayout(form);

    QHBoxLayout *btns = new QHBoxLayout();
    QPushButton *startBtn = new QPushButton("Start Game", this);
    QPushButton *cancelBtn = new QPushButton("Cancel", this);
    btns->addWidget(startBtn);
    btns->addWidget(cancelBtn);
    root->addLayout(btns);

    QObject::connect(startBtn, &QPushButton::clicked, [this]() {
        accept();
    });
    QObject::connect(cancelBtn, &QPushButton::clicked, [this]() {
        reject();
    });

    setTheme(_theme);
}

NewLocalGameDialog::~NewLocalGameDialog() = default;

void NewLocalGameDialog::setTheme(ThemeId theme)
{
    _theme = theme;
    setStyleSheet(buildAppStyleSheet(_theme));
}

ThemeId NewLocalGameDialog::theme() const
{
    return _theme;
}

void NewLocalGameDialog::setConfig(const LocalGameConfig &cfg)
{
    int pIdx = _playTypeCombo->findData(static_cast<int>(cfg.playType));
    if (pIdx >= 0) {
        _playTypeCombo->setCurrentIndex(pIdx);
    }
    int dIdx = _difficultyCombo->findData(static_cast<int>(cfg.botDifficulty));
    if (dIdx >= 0) {
        _difficultyCombo->setCurrentIndex(dIdx);
    }
    int sIdx = _humanSeatCombo->findData(static_cast<int>(cfg.humanSeat));
    if (sIdx >= 0) {
        _humanSeatCombo->setCurrentIndex(sIdx);
    }
    int mIdx = _modeCombo->findData(static_cast<int>(cfg.mode));
    if (mIdx >= 0) {
        _modeCombo->setCurrentIndex(mIdx);
    }
    int bIdx = _basesCombo->findData(static_cast<int>(cfg.bases));
    if (bIdx >= 0) {
        _basesCombo->setCurrentIndex(bIdx);
    }
    _superProngsCheck->setChecked(cfg.useSuperProngs);
    _edgelessCheck->setChecked(cfg.useEdgelessBoard);
    _timeControlCheck->setChecked(cfg.timeControlEnabled);
    if (cfg.initialTimeMs > 0) {
        _initialSecSpin->setValue(static_cast<int>(cfg.initialTimeMs / 1000));
    }
    _incrementSecSpin->setValue(static_cast<int>(cfg.incrementMs / 1000));
}

LocalGameConfig NewLocalGameDialog::config() const
{
    LocalGameConfig cfg;
    cfg.playType = static_cast<LocalPlayType>(
        _playTypeCombo->currentData().toInt());
    cfg.botDifficulty = static_cast<BotDifficulty>(
        _difficultyCombo->currentData().toInt());
    cfg.humanSeat = static_cast<OctiPlayer>(
        _humanSeatCombo->currentData().toInt());
    cfg.mode = static_cast<OctiGameMode>(_modeCombo->currentData().toInt());
    cfg.bases = static_cast<OctiGameBases>(_basesCombo->currentData().toInt());
    cfg.useSuperProngs = _superProngsCheck->isChecked();
    cfg.useEdgelessBoard = _edgelessCheck->isChecked();
    cfg.timeControlEnabled = _timeControlCheck->isChecked();
    cfg.initialTimeMs =
        static_cast<long>(_initialSecSpin->value()) * 1000L;
    cfg.incrementMs =
        static_cast<long>(_incrementSecSpin->value()) * 1000L;
    return cfg;
}

GameOverDialog::GameOverDialog(const std::string &winnerBanner,
                               const std::string &terminationReason,
                               int moveCount,
                               const std::string &octiTranscript,
                               ThemeId theme,
                               QWidget *parent)
    : QDialog(parent),
      _theme(theme),
      _winnerBanner(QString::fromStdString(winnerBanner)),
      _terminationReason(QString::fromStdString(terminationReason)),
      _moveCount(moveCount),
      _transcript(octiTranscript),
      _rematchRequested(false),
      _newGameRequested(false),
      _newGameBtnRect(56.0, 210.0, 172.0, 42.0),
      _reviewBoardBtnRect(252.0, 210.0, 172.0, 42.0)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog);
    setAttribute(Qt::WA_TranslucentBackground, true);
    setFixedSize(480, 280);
    setModal(true);
    setWindowTitle("OCTI — Game Over");
}

GameOverDialog::GameOverDialog(const OctiBoard *board,
                               ThemeId theme,
                               QWidget *parent)
    : GameOverDialog(describeWinnerBanner(board).toStdString(),
                     describeTerminationReason(board).toStdString(),
                     (board && board->getGame())
                         ? board->getGame()->getNumMoves()
                         : 0,
                     "",
                     theme,
                     parent)
{
}

GameOverDialog::~GameOverDialog() = default;

QString GameOverDialog::describeWinnerBanner(const OctiBoard *board)
{
    if (!board || !board->getGame()) {
        return QString::fromUtf8("GAME OVER");
    }
    OctiPlayer winner = board->getGame()->getWinner();
    if (board->getGameMode() == GAME_MODE_4) {
        if (winner == PLAYER_A || winner == PLAYER_C) {
            return QString::fromUtf8("VICTORY \xE2\x80\x94 TEAM A+C WINS!");
        }
        if (winner == PLAYER_B || winner == PLAYER_D) {
            return QString::fromUtf8("VICTORY \xE2\x80\x94 TEAM B+D WINS!");
        }
    } else {
        if (winner == PLAYER_A) {
            return QString::fromUtf8("VICTORY \xE2\x80\x94 PLAYER A WINS!");
        }
        if (winner == PLAYER_B) {
            return QString::fromUtf8("VICTORY \xE2\x80\x94 PLAYER B WINS!");
        }
        if (winner == PLAYER_C) {
            return QString::fromUtf8("VICTORY \xE2\x80\x94 PLAYER C WINS!");
        }
        if (winner == PLAYER_D) {
            return QString::fromUtf8("VICTORY \xE2\x80\x94 PLAYER D WINS!");
        }
    }
    return QString::fromUtf8("DRAW / STALEMATE");
}

QString GameOverDialog::describeTerminationReason(const OctiBoard *board)
{
    if (!board || !board->getGame()) {
        return QStringLiteral("Match Concluded");
    }
    if (board->getGame()->isGameResigned()) {
        return QStringLiteral("Victory by Resignation");
    }
    std::vector<std::pair<OctiPlayer, OctiMove>> moves;
    board->getGame()->getMoves(moves);
    if (!moves.empty() && moves.back().second.getOps().empty()) {
        return QStringLiteral("Victory by Resignation");
    }

    OctiPlayer winner = board->getGame()->getWinner();
    if (winner != PLAYER_UNDEF) {
        for (int x = 0; x < 9; ++x) {
            for (int y = 0; y < 9; ++y) {
                const OctiSquare &sq = board->squareAt(x, y);
                if (sq.isEmpty()) {
                    continue;
                }
                OctiSquareType st = sq.getType();
                OctiPodType wPod = (winner == PLAYER_A ? A_POD :
                                    winner == PLAYER_B ? B_POD :
                                    winner == PLAYER_C ? C_POD : D_POD);
                bool hasWinnerPod = false;
                for (int pi = 0; pi < sq.getNumPods(); ++pi) {
                    if (sq.getPodAt(pi).getType() == wPod) {
                        hasWinnerPod = true;
                        break;
                    }
                }
                if (!hasWinnerPod) {
                    continue;
                }
                if ((st == A_HOME_SQUARE && winner != PLAYER_A) ||
                    (st == B_HOME_SQUARE && winner != PLAYER_B) ||
                    (st == C_HOME_SQUARE && winner != PLAYER_C) ||
                    (st == D_HOME_SQUARE && winner != PLAYER_D)) {
                    return QStringLiteral("Captured Enemy Home Base");
                }
            }
        }
        OctiPlayer opp = (winner == PLAYER_A) ? PLAYER_B : PLAYER_A;
        if (board->getGame()->isPlayerEliminated(opp)) {
            return QStringLiteral("Eliminated All Opponent Pods");
        }
    }
    return QStringLiteral("Captured Enemy Home Base");
}

void GameOverDialog::setTheme(ThemeId theme)
{
    _theme = theme;
    update();
}

ThemeId GameOverDialog::theme() const
{
    return _theme;
}

QString GameOverDialog::winnerBannerText() const
{
    return _winnerBanner;
}

QString GameOverDialog::terminationText() const
{
    return QString("Termination: %1").arg(_terminationReason);
}

int GameOverDialog::moveCount() const
{
    return _moveCount;
}

const std::string &GameOverDialog::octiTranscript() const
{
    return _transcript;
}

bool GameOverDialog::exportTranscriptToFile(const std::string &filePath) const
{
    if (filePath.empty()) {
        return false;
    }
    std::ofstream ofs(filePath, std::ios::out | std::ios::trunc);
    if (!ofs.is_open()) {
        return false;
    }
    ofs << _transcript;
    return ofs.good();
}

void GameOverDialog::triggerRematch()
{
    _rematchRequested = true;
    _newGameRequested = true;
    if (_onRematchCallback) {
        _onRematchCallback();
    }
    accept();
}

bool GameOverDialog::wasRematchRequested() const
{
    return _rematchRequested;
}

bool GameOverDialog::newGameRequested() const
{
    return _newGameRequested;
}

void GameOverDialog::setOnRematchCallback(std::function<void()> cb)
{
    _onRematchCallback = std::move(cb);
}

QImage GameOverDialog::renderToImage(int width, int height)
{
    QImage img(width, height, QImage::Format_ARGB32_Premultiplied);
    img.fill(Qt::transparent);
    QPainter p(&img);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::TextAntialiasing, true);

    const OctiPalette &pal = paletteForTheme(_theme);

    QRectF outerRect(2.0, 2.0, width - 4.0, height - 4.0);
    p.setPen(QPen(pal.prongSuperGold, 2.4));
    p.setBrush(pal.panelCardBg);
    p.drawRoundedRect(outerRect, 12.0, 12.0);

    QRectF innerRect = outerRect.adjusted(6.0, 6.0, -6.0, -6.0);
    p.setPen(QPen(pal.accentBrass, 1.0));
    p.setBrush(Qt::NoBrush);
    p.drawRoundedRect(innerRect, 8.0, 8.0);

    QFont badgeFont("SansSerif", 9, QFont::Bold);
    badgeFont.setLetterSpacing(QFont::AbsoluteSpacing, 2.0);
    p.setFont(badgeFont);
    p.setPen(pal.accentBrass);
    p.drawText(QRectF(24.0, 22.0, width - 48.0, 20.0),
               Qt::AlignCenter,
               QStringLiteral("MATCH CONCLUDED"));

    QFont bannerFont("SansSerif", 17, QFont::Bold);
    p.setFont(bannerFont);
    p.setPen(pal.prongSuperGold);
    p.drawText(QRectF(24.0, 52.0, width - 48.0, 44.0),
               Qt::AlignCenter | Qt::TextWordWrap,
               _winnerBanner);

    p.setPen(QPen(pal.accentBrass.darker(130), 1.0));
    p.drawLine(QPointF(60.0, 106.0), QPointF(width - 60.0, 106.0));

    QFont reasonFont("SansSerif", 12, QFont::DemiBold);
    p.setFont(reasonFont);
    p.setPen(pal.textPrimary);
    p.drawText(QRectF(24.0, 118.0, width - 48.0, 28.0),
               Qt::AlignCenter,
               _terminationReason);

    QFont statsFont("SansSerif", 10, QFont::Normal);
    p.setFont(statsFont);
    p.setPen(pal.textSecondary);
    p.drawText(QRectF(24.0, 150.0, width - 48.0, 24.0),
               Qt::AlignCenter,
               QString("Total Plies / Moves: %1").arg(_moveCount));

    double btnW = 176.0;
    double btnH = 42.0;
    double gap = 24.0;
    double startX = (width - (2.0 * btnW + gap)) * 0.5;
    double btnY = height - btnH - 26.0;

    _newGameBtnRect = QRectF(startX, btnY, btnW, btnH);
    _reviewBoardBtnRect = QRectF(startX + btnW + gap, btnY, btnW, btnH);

    p.setPen(QPen(pal.prongSuperGold, 1.6));
    p.setBrush(pal.podA.core);
    p.drawRoundedRect(_newGameBtnRect, 7.0, 7.0);
    p.setFont(QFont("SansSerif", 11, QFont::Bold));
    p.setPen(Qt::white);
    p.drawText(_newGameBtnRect, Qt::AlignCenter, QStringLiteral("New Game"));

    p.setPen(QPen(pal.accentBrass, 1.4));
    p.setBrush(pal.panelCardBg.darker(130));
    p.drawRoundedRect(_reviewBoardBtnRect, 7.0, 7.0);
    p.setPen(pal.textPrimary);
    p.drawText(_reviewBoardBtnRect, Qt::AlignCenter, QStringLiteral("Review Board"));

    p.end();
    return img;
}

void GameOverDialog::paintEvent(QPaintEvent *event)
{
    (void)event;
    QPainter p(this);
    p.drawImage(0, 0, renderToImage(width(), height()));
}

void GameOverDialog::mousePressEvent(QMouseEvent *event)
{
    if (!event) {
        return;
    }
    QPointF pos = mousePosF(event);
    if (_newGameBtnRect.contains(pos)) {
        triggerRematch();
        return;
    }
    if (_reviewBoardBtnRect.contains(pos)) {
        _newGameRequested = false;
        accept();
        return;
    }
}

void GameOverDialog::keyPressEvent(QKeyEvent *event)
{
    if (!event) {
        return;
    }
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        triggerRematch();
        return;
    }
    if (event->key() == Qt::Key_Escape) {
        _newGameRequested = false;
        reject();
        return;
    }
    QDialog::keyPressEvent(event);
}

/* ==================================================================== */
/* 8. SelfUpdateDialog                                                  */
/* ==================================================================== */

SelfUpdateDialog::SelfUpdateDialog(
    std::shared_ptr<selfupdate::SelfUpdateController> controller,
    ThemeId theme,
    QWidget *parent)
    : QDialog(parent),
      _controller(std::move(controller)),
      _ownerWindow(dynamic_cast<OctiMainWindow *>(parent)),
      _boundStandalone(false),
      _theme(theme),
      _currentState(),
      _phaseBadgeLabel(nullptr),
      _versionSummaryLabel(nullptr),
      _metadataLabel(nullptr),
      _releaseNotesView(nullptr),
      _progressBar(nullptr),
      _statusLabel(nullptr),
      _checkBtn(nullptr),
      _downloadInstallBtn(nullptr),
      _cancelBtn(nullptr),
      _closeBtn(nullptr)
{
    setWindowTitle("Software Update — OCTI Desktop Player");
    setMinimumSize(540, 480);
    resize(580, 520);
    setModal(true);

    QVBoxLayout *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(16, 16, 16, 16);
    rootLayout->setSpacing(12);

    QHBoxLayout *headerRow = new QHBoxLayout();
    headerRow->setSpacing(10);

    QLabel *iconLabel = new QLabel(this);
    iconLabel->setPixmap(QPixmap::fromImage(renderOctiIconImage(40)));
    iconLabel->setFixedSize(40, 40);
    headerRow->addWidget(iconLabel);

    QVBoxLayout *headerTextLayout = new QVBoxLayout();
    headerTextLayout->setSpacing(2);
    QLabel *titleLabel = new QLabel("OCTI Desktop Player", this);
    titleLabel->setStyleSheet("font-size: 16px; font-weight: bold;");
    _versionSummaryLabel = new QLabel(this);
    _versionSummaryLabel->setStyleSheet("font-size: 13px;");
    headerTextLayout->addWidget(titleLabel);
    headerTextLayout->addWidget(_versionSummaryLabel);
    headerRow->addLayout(headerTextLayout, 1);

    _phaseBadgeLabel = new QLabel("IDLE", this);
    _phaseBadgeLabel->setStyleSheet(
        "font-size: 11px; font-weight: bold; padding: 4px 8px; "
        "border-radius: 4px; border: 1px solid #C8A253;");
    headerRow->addWidget(_phaseBadgeLabel, 0, Qt::AlignTop);
    rootLayout->addLayout(headerRow);

    _metadataLabel = new QLabel(this);
    _metadataLabel->setWordWrap(true);
    _metadataLabel->setStyleSheet("font-size: 11px;");
    rootLayout->addWidget(_metadataLabel);

    QGroupBox *notesBox = new QGroupBox("Release Notes", this);
    QVBoxLayout *notesLayout = new QVBoxLayout(notesBox);
    _releaseNotesView = new QPlainTextEdit(notesBox);
    _releaseNotesView->setReadOnly(true);
    notesLayout->addWidget(_releaseNotesView);
    rootLayout->addWidget(notesBox, 1);

    _progressBar = new QProgressBar(this);
    _progressBar->setRange(0, 100);
    _progressBar->setValue(0);
    _progressBar->setFixedHeight(20);
    rootLayout->addWidget(_progressBar);

    _statusLabel = new QLabel(this);
    _statusLabel->setWordWrap(true);
    _statusLabel->setStyleSheet("font-size: 12px;");
    rootLayout->addWidget(_statusLabel);

    QHBoxLayout *btnRow = new QHBoxLayout();
    btnRow->setSpacing(8);

    _checkBtn = new QPushButton("Check for Updates", this);
    _downloadInstallBtn = new QPushButton("Download && Update Now", this);
    _cancelBtn = new QPushButton("Cancel", this);
    _closeBtn = new QPushButton("Close", this);

    btnRow->addWidget(_checkBtn);
    btnRow->addWidget(_downloadInstallBtn);
    btnRow->addWidget(_cancelBtn);
    btnRow->addStretch(1);
    btnRow->addWidget(_closeBtn);
    rootLayout->addLayout(btnRow);

    QObject::connect(_checkBtn, &QPushButton::clicked, [this]() {
        triggerCheckForUpdates();
    });
    QObject::connect(_downloadInstallBtn, &QPushButton::clicked, [this]() {
        triggerDownloadAndInstall();
    });
    QObject::connect(_cancelBtn, &QPushButton::clicked, [this]() {
        triggerCancelDownload();
    });
    QObject::connect(_closeBtn, &QPushButton::clicked, [this]() {
        reject();
    });

    setTheme(_theme);

    if (_ownerWindow != nullptr) {
        _ownerWindow->registerUpdateDialog(this);
        applyUpdateState(_ownerWindow->currentSelfUpdateState());
    } else if (_controller != nullptr) {
        _boundStandalone = true;
        selfupdate::bindQtController(
            *_controller,
            this,
            [this](const selfupdate::UpdateState &st) {
                applyUpdateState(st);
            });
        applyUpdateState(_controller->state());
    } else {
        applyUpdateState(_currentState);
    }
}

SelfUpdateDialog::~SelfUpdateDialog()
{
    if (_ownerWindow != nullptr) {
        _ownerWindow->unregisterUpdateDialog(this);
    } else if (_boundStandalone && _controller != nullptr) {
        _controller->setStateCallback(nullptr);
    }
}

void SelfUpdateDialog::setTheme(ThemeId theme)
{
    _theme = theme;
    setStyleSheet(buildAppStyleSheet(_theme));
}

ThemeId SelfUpdateDialog::theme() const
{
    return _theme;
}

void SelfUpdateDialog::applyUpdateState(const selfupdate::UpdateState &state)
{
    _currentState = state;

    QString installedVer = QString::fromStdString(
        state.installedVersionName.empty() ? OctiCore::versionString()
                                           : state.installedVersionName);

    switch (state.phase) {
    case selfupdate::UpdatePhase::Idle:
        _phaseBadgeLabel->setText("IDLE");
        _versionSummaryLabel->setText(
            QString("Installed: v%1").arg(installedVer));
        _metadataLabel->setText("");
        _releaseNotesView->setPlainText("No release notes.");
        _statusLabel->setText(
            "Ready to check https://www.selfso.com/software/windows/ for updates.");
        _checkBtn->setEnabled(true);
        _downloadInstallBtn->setEnabled(false);
        _downloadInstallBtn->setText("Download && Update Now");
        _cancelBtn->setEnabled(false);
        _progressBar->setValue(0);
        break;

    case selfupdate::UpdatePhase::Checking:
        _phaseBadgeLabel->setText("CHECKING");
        _statusLabel->setText("Querying Selfso Windows release catalog...");
        _checkBtn->setEnabled(false);
        _downloadInstallBtn->setEnabled(false);
        _cancelBtn->setEnabled(true);
        break;

    case selfupdate::UpdatePhase::UpToDate:
        _phaseBadgeLabel->setText("UP TO DATE");
        _versionSummaryLabel->setText(
            QString("Installed: v%1 (Latest)").arg(installedVer));
        _statusLabel->setText(
            QString("OCTI Desktop Player v%1 is up to date (No updates available).")
                .arg(installedVer));
        _checkBtn->setEnabled(true);
        _downloadInstallBtn->setEnabled(true);
        _downloadInstallBtn->setText("Reinstall");
        _cancelBtn->setEnabled(false);
        _progressBar->setValue(100);
        break;

    case selfupdate::UpdatePhase::Available: {
        _phaseBadgeLabel->setText("UPDATE AVAILABLE");
        QString availVer = QString::fromStdString(state.manifest.versionName);
        _versionSummaryLabel->setText(
            QString("Installed: v%1 -> Available: v%2 (Build %3)")
                .arg(installedVer)
                .arg(availVer)
                .arg(state.manifest.versionCode));

        QString modeStr = (state.manifest.installMode == selfupdate::InstallMode::Installer)
                              ? "Setup Installer"
                              : "Standalone Binary";
        QString sizeStr = (state.manifest.sizeBytes > 0)
                              ? QString("%1 KB (%2 bytes)")
                                    .arg(state.manifest.sizeBytes / 1024)
                                    .arg(state.manifest.sizeBytes)
                              : "Unknown size";
        _metadataLabel->setText(
            QString("Mode: %1 | Architecture: %2 | Minimum OS: %3\nSize: %4 | SHA-256: %5")
                .arg(modeStr)
                .arg(QString::fromStdString(state.manifest.arch))
                .arg(QString::fromStdString(state.manifest.minOs))
                .arg(sizeStr)
                .arg(QString::fromStdString(state.manifest.sha256)));

        if (!state.manifest.releaseNotes.empty()) {
            _releaseNotesView->setPlainText(
                QString::fromStdString(state.manifest.releaseNotes));
        } else {
            _releaseNotesView->setPlainText("No release notes provided.");
        }

        _statusLabel->setText("New version is available for download.");
        _downloadInstallBtn->setEnabled(true);
        _downloadInstallBtn->setText("Download && Update Now");
        _checkBtn->setEnabled(true);
        _cancelBtn->setEnabled(false);
        _progressBar->setValue(0);
        break;
    }

    case selfupdate::UpdatePhase::Downloading: {
        _phaseBadgeLabel->setText("DOWNLOADING");
        int pct = 0;
        double frac = state.progressFraction();
        if (frac >= 0.0) {
            pct = std::clamp(static_cast<int>(frac * 100.0), 0, 100);
        }
        _progressBar->setValue(pct);
        _statusLabel->setText(
            QString("Downloading update package... %1% (%2 / %3 bytes)")
                .arg(pct)
                .arg(state.bytesRead)
                .arg(state.totalBytes > 0 ? state.totalBytes : 0));
        _downloadInstallBtn->setEnabled(false);
        _checkBtn->setEnabled(false);
        _cancelBtn->setEnabled(true);
        break;
    }

    case selfupdate::UpdatePhase::ReadyToInstall:
        _phaseBadgeLabel->setText("READY TO INSTALL");
        _progressBar->setValue(100);
        _statusLabel->setText("Package verified (SHA-256 OK). Ready to install.");
        _downloadInstallBtn->setEnabled(true);
        _downloadInstallBtn->setText("Install && Relaunch Now");
        _checkBtn->setEnabled(true);
        _cancelBtn->setEnabled(false);
        break;

    case selfupdate::UpdatePhase::Installing:
        _phaseBadgeLabel->setText("INSTALLING");
        _statusLabel->setText("Launching update installer...");
        _downloadInstallBtn->setEnabled(false);
        _checkBtn->setEnabled(false);
        _cancelBtn->setEnabled(false);
        break;

    case selfupdate::UpdatePhase::Relaunching:
        _phaseBadgeLabel->setText("RELAUNCHING");
        _progressBar->setValue(100);
        _statusLabel->setText(
            "Update staged. Closing OCTI to complete installation and relaunch...");
        _checkBtn->setEnabled(false);
        _downloadInstallBtn->setEnabled(false);
        _cancelBtn->setEnabled(false);
        _closeBtn->setEnabled(false);
        if (isVisible()) {
            accept();
        }
        break;

    case selfupdate::UpdatePhase::Error:
        _phaseBadgeLabel->setText("UPDATE ERROR");
        _statusLabel->setText(
            QString("Error: %1").arg(QString::fromStdString(state.errorMessage)));
        _checkBtn->setEnabled(true);
        _downloadInstallBtn->setEnabled(!state.manifest.packageUrl.empty());
        _cancelBtn->setEnabled(false);
        break;
    }
}

const selfupdate::UpdateState &SelfUpdateDialog::currentUpdateState() const
{
    return _currentState;
}

void SelfUpdateDialog::triggerCheckForUpdates()
{
    if (_ownerWindow) {
        _ownerWindow->checkForAppUpdates(true);
    } else if (_controller) {
        _controller->checkForUpdates(true);
    }
}

void SelfUpdateDialog::triggerDownloadAndInstall()
{
    if (_ownerWindow) {
        _ownerWindow->triggerAppUpdateDownloadAndInstall();
    } else if (_controller) {
        if (_currentState.phase == selfupdate::UpdatePhase::ReadyToInstall) {
            _controller->installStagedPackage();
        } else if (_currentState.phase == selfupdate::UpdatePhase::UpToDate) {
            _controller->reinstall();
        } else {
            _controller->downloadAndInstall();
        }
    }
}

void SelfUpdateDialog::triggerCancelDownload()
{
    if (_ownerWindow) {
        _ownerWindow->cancelAppUpdateDownload();
    } else if (_controller) {
        _controller->cancel();
    }
}

QString SelfUpdateDialog::phaseBadgeText() const
{
    return _phaseBadgeLabel ? _phaseBadgeLabel->text() : QString();
}

QString SelfUpdateDialog::versionSummaryText() const
{
    return _versionSummaryLabel ? _versionSummaryLabel->text() : QString();
}

QString SelfUpdateDialog::releaseNotesText() const
{
    return _releaseNotesView ? _releaseNotesView->toPlainText() : QString();
}

QString SelfUpdateDialog::metadataSummaryText() const
{
    return _metadataLabel ? _metadataLabel->text() : QString();
}

QString SelfUpdateDialog::statusText() const
{
    return _statusLabel ? _statusLabel->text() : QString();
}

int SelfUpdateDialog::progressPercent() const
{
    return _progressBar ? _progressBar->value() : 0;
}

bool SelfUpdateDialog::isDownloadInstallButtonEnabled() const
{
    return _downloadInstallBtn ? _downloadInstallBtn->isEnabled() : false;
}

QString SelfUpdateDialog::downloadInstallButtonText() const
{
    return _downloadInstallBtn ? _downloadInstallBtn->text() : QString();
}

bool SelfUpdateDialog::isCancelButtonEnabled() const
{
    return _cancelBtn ? _cancelBtn->isEnabled() : false;
}

/* ==================================================================== */
/* OctiSplashDialog                                                     */
/* ==================================================================== */

OctiSplashDialog::OctiSplashDialog(QWidget *parent, ThemeId theme)
    : QDialog(parent)
    , _theme(theme)
    , _timer(this)
    , _elapsed()
    , _progress(0.0)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog);
    setAttribute(Qt::WA_TranslucentBackground, false);
    setFixedSize(960, 560);
    setModal(true);

    _elapsed.start();
    QObject::connect(&_timer, &QTimer::timeout, this, [this]() {
        qint64 ms = _elapsed.elapsed();
        _progress = std::clamp(static_cast<double>(ms) / 10000.0, 0.0, 1.0);
        update();
        if (ms >= 10000) {
            _timer.stop();
            accept();
        }
    });
    _timer.start(16);
}

OctiSplashDialog::~OctiSplashDialog()
{
    if (_timer.isActive()) {
        _timer.stop();
    }
}

QImage OctiSplashDialog::renderSplashFrame(int width, int height, double progress)
{
    progress = std::clamp(progress, 0.0, 1.0);
    QImage frame(width, height, QImage::Format_ARGB32_Premultiplied);
    frame.fill(Qt::transparent);
    QPainter p(&frame);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::TextAntialiasing, true);

    const OctiPalette &pal = paletteForTheme(ThemeId::OBSIDIAN_BRASS);

    double w = static_cast<double>(width);
    double h = static_cast<double>(height);

    // 1. Radial background
    QPointF bgCenter(w * 0.5, h * 0.42);
    QRadialGradient bgGrad(bgCenter, std::max(w, h) * 0.75);
    bgGrad.setColorAt(0.0, QColor(24, 34, 52));
    bgGrad.setColorAt(0.55, QColor(13, 18, 27));
    bgGrad.setColorAt(1.0, QColor(5, 7, 11));
    p.fillRect(QRectF(0, 0, w, h), bgGrad);

    // 2. Animated perspective tabletop grid
    p.save();
    double vpX = w * 0.5;
    double vpY = h * 0.12;
    for (int i = -12; i <= 12; ++i) {
        double botX = w * 0.5 + i * (w * 0.065);
        p.setPen(QPen(QColor(212, 175, 55, 30), 1.0));
        p.drawLine(QPointF(vpX, vpY), QPointF(botX, h));
    }
    double scrollY = std::fmod(progress * 180.0, 36.0);
    for (double y = h * 0.35 + scrollY; y < h; y += 36.0) {
        double factor = (y - vpY) / (h - vpY);
        p.setPen(QPen(QColor(212, 175, 55, static_cast<int>(35 * factor)), 1.0));
        p.drawLine(QPointF(0, y), QPointF(w, y));
    }
    p.restore();

    // 3. Rotating brass compass ring
    p.save();
    QPointF podCenter(w * 0.5, h * 0.40);
    double outerR = std::min(w, h) * 0.28;
    double ringAngleDeg = progress * 360.0 * 0.4;
    p.translate(podCenter);
    p.rotate(ringAngleDeg);

    QPen brassRingPen(pal.boardOuterFrame, 2.2);
    p.setPen(brassRingPen);
    p.setBrush(Qt::NoBrush);
    p.drawEllipse(QPointF(0, 0), outerR, outerR);
    p.setPen(QPen(pal.accentBrass, 1.2, Qt::DashLine));
    p.drawEllipse(QPointF(0, 0), outerR * 0.85, outerR * 0.85);

    for (int deg = 0; deg < 360; deg += 15) {
        double rad = deg * M_PI / 180.0;
        double r1 = (deg % 45 == 0) ? (outerR * 0.80) : (outerR * 0.86);
        double r2 = outerR * 0.98;
        p.setPen(QPen(pal.accentBrass, (deg % 90 == 0) ? 2.0 : 1.0));
        p.drawLine(QPointF(std::cos(rad) * r1, std::sin(rad) * r1),
                   QPointF(std::cos(rad) * r2, std::sin(rad) * r2));
    }
    p.restore();

    // 4. Central animated jewel-faceted Emerald & Brass OCTI Pod & Prongs
    p.save();
    double podScale = 0.5 + 0.5 * std::min(1.0, progress * 1.6);
    double podR = std::min(w, h) * 0.16 * podScale;
    QRectF podRect(podCenter.x() - podR, podCenter.y() - podR, podR * 2.0, podR * 2.0);

    OctiPod pod(A_POD);
    if (progress >= 0.25) pod.addProng(DIR_E, false);
    if (progress >= 0.40) pod.addProng(DIR_W, false);
    if (progress >= 0.55) pod.addProng(DIR_NE, false);
    if (progress >= 0.70) pod.addProng(DIR_SW, false);
    if (progress >= 0.82) pod.addProng(DIR_N, true); // crowned Superprong!

    int podRotDeg = static_cast<int>((1.0 - std::min(1.0, progress * 1.5)) * 45.0);
    paintOctagonalPod(p, podRect, A_POD, pod, pal, podRotDeg, true, 1.0);

    // Glowing shockwave rings when prongs snap in
    if (progress >= 0.82) {
        double shockR = podR * (1.1 + 0.35 * (progress - 0.82) / 0.18);
        p.setPen(QPen(pal.prongSuperGold, 2.5));
        p.setBrush(Qt::NoBrush);
        p.drawEllipse(podCenter, shockR, shockR);
    } else if (progress >= 0.55) {
        double shockR = podR * (1.05 + 0.25 * (progress - 0.55) / 0.27);
        p.setPen(QPen(pal.legalStepGuide, 1.8));
        p.setBrush(Qt::NoBrush);
        p.drawEllipse(podCenter, shockR, shockR);
    }

    // Orbiting particles
    int numParticles = 8;
    for (int i = 0; i < numParticles; ++i) {
        double partAngle = (progress * 2.5 * M_PI) + (i * 2.0 * M_PI / numParticles);
        double partR = podR * (1.35 + 0.15 * std::sin(progress * 6.0 + i));
        QPointF partPos(podCenter.x() + std::cos(partAngle) * partR,
                        podCenter.y() + std::sin(partAngle) * partR);
        p.setPen(Qt::NoPen);
        p.setBrush((i % 2 == 0) ? pal.prongSuperGold : pal.accentBrass);
        p.drawEllipse(partPos, 2.5, 2.5);
    }
    p.restore();

    // 5. Typography (Title, Subtitle, Version, Credits, Copyright)
    p.save();
    QFont titleFont("SansSerif", 36, QFont::Bold);
    titleFont.setLetterSpacing(QFont::AbsoluteSpacing, 6.0);
    p.setFont(titleFont);
    QRectF titleRect(0, h * 0.04, w, 48);
    p.setPen(QColor(10, 14, 20, 220));
    p.drawText(titleRect.translated(2.0, 2.0), Qt::AlignCenter, "OCTI");
    p.setPen(pal.prongSuperGold);
    p.drawText(titleRect, Qt::AlignCenter, "OCTI");

    QFont subFont("SansSerif", 13, QFont::Medium);
    subFont.setLetterSpacing(QFont::AbsoluteSpacing, 1.5);
    p.setFont(subFont);
    p.setPen(pal.accentBrass);
    QRectF subRect(0, h * 0.04 + 48, w, 24);
    p.drawText(subRect, Qt::AlignCenter, "Strategic Abstract Board Game Platform");

    double metaY = h * 0.67;
    QFont metaFont("SansSerif", 11, QFont::Bold);
    p.setFont(metaFont);
    p.setPen(pal.textPrimary);
    QRectF verRect(0, metaY, w, 22);
    p.drawText(verRect, Qt::AlignCenter,
               QString("Version %1 (Protocol v1)")
                   .arg(QString::fromStdString(OctiCore::versionString())));

    QFont credFont("SansSerif", 10);
    p.setFont(credFont);
    p.setPen(QColor(185, 195, 210));
    QRectF authRect(0, metaY + 24, w, 20);
    p.drawText(authRect, Qt::AlignCenter, "Author: Charles Chiou (selfso.com)");

    QRectF gameRect(0, metaY + 44, w, 20);
    p.drawText(gameRect, Qt::AlignCenter, "Original OCTI Game Designed by Don Green");

    QRectF cprtRect(0, metaY + 64, w, 20);
    p.setPen(pal.accentBrass);
    p.drawText(cprtRect, Qt::AlignCenter,
               "Copyright (C) 2026, Charles Chiou. All Rights Reserved.");

    double barW = w * 0.55;
    double barH = 5.0;
    QRectF barBg(w * 0.5 - barW * 0.5, h - 34.0, barW, barH);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(30, 40, 56));
    p.drawRoundedRect(barBg, 2.5, 2.5);

    QRectF barFill(barBg.left(), barBg.top(), barW * progress, barH);
    QLinearGradient barGrad(barFill.topLeft(), barFill.topRight());
    barGrad.setColorAt(0.0, pal.podA.core);
    barGrad.setColorAt(1.0, pal.prongSuperGold);
    p.setBrush(barGrad);
    p.drawRoundedRect(barFill, 2.5, 2.5);

    QFont hintFont("SansSerif", 9);
    p.setFont(hintFont);
    p.setPen(QColor(150, 160, 175));
    QRectF hintRect(0, h - 24.0, w, 18);
    p.drawText(hintRect, Qt::AlignCenter, "Click or Press ESC to Continue");

    p.restore();
    return frame;
}

void OctiSplashDialog::paintEvent(QPaintEvent *event)
{
    (void)event;
    QPainter p(this);
    p.drawImage(0, 0, renderSplashFrame(width(), height(), _progress));
}

void OctiSplashDialog::keyPressEvent(QKeyEvent *event)
{
    if (event && event->key() == Qt::Key_Escape) {
        if (_timer.isActive()) {
            _timer.stop();
        }
        reject();
        return;
    }
    QDialog::keyPressEvent(event);
}

void OctiSplashDialog::mousePressEvent(QMouseEvent *event)
{
    (void)event;
    if (_timer.isActive()) {
        _timer.stop();
    }
    accept();
}

/* ==================================================================== */
/* ZoomedSquareDialog                                                   */
/* ==================================================================== */

ZoomedSquareDialog::ZoomedSquareDialog(OctiBoard *board,
                                       OctiMoveComposer *composer,
                                       int squareX, int squareY,
                                       BoardRotation rotation,
                                       ThemeId theme,
                                       QWidget *parent)
    : QDialog(parent)
    , _board(board)
    , _composer(composer)
    , _squareX(squareX)
    , _squareY(squareY)
    , _rotation(rotation)
    , _theme(theme)
    , _selectedPodIndex(0)
    , _shouldMovePod(false)
    , _moveAllMatchingStack(false)
    , _actionCompleted(false)
    , _socketRadius(18.0)
    , _selectedProngType(SelectedProngType::NORMAL)
    , _selectedProngSocketForReposition(DIR_UNDEF)
    , _dragKind(ZoomDragKind::NONE)
    , _dragSrcProngDir(DIR_UNDEF)
    , _hoverProngDir(DIR_UNDEF)
    , _dragCursorPos()
{
    setWindowFlags(Qt::Popup | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
    setAttribute(Qt::WA_TranslucentBackground, true);
    setFixedSize(560, 530);
    setModal(true);
    setWindowTitle(QString("Square (%1, %2) Close-Up Inspector").arg(_squareX).arg(_squareY));

    if (_board && _board->isValidCoord(_squareX - 1, _squareY - 1)) {
        const OctiSquare &sq = _board->squareAt(_squareX - 1, _squareY - 1);
        if (_composer) {
            _selectedPodIndex = _composer->selectedStackPodIndex(_squareX, _squareY);
        }
        if (_selectedPodIndex < 0 || _selectedPodIndex >= sq.getNumPods()) {
            _selectedPodIndex = std::max(0, sq.getNumPods() - 1);
        }
        if (_composer && sq.getNumPods() > 0 &&
            _composer->isSquarePodClaimed(_squareX, _squareY, _selectedPodIndex)) {
            int unclaimed = _composer->firstUnclaimedFriendlyPodIndex(_squareX, _squareY);
            if (unclaimed >= 0 && unclaimed < sq.getNumPods()) {
                _selectedPodIndex = unclaimed;
                _composer->selectStackPodIndex(_squareX, _squareY, _selectedPodIndex);
            }
        }
        if (_board->getGame()) {
            OctiPlayer t = _board->getGame()->getTurn();
            if (_board->getGame()->getFreeProngs(t) == 0 &&
                _board->getGame()->getFreeSuperProngs(t) > 0) {
                _selectedProngType = SelectedProngType::SUPER;
            }
        }
    }
}

ZoomedSquareDialog::~ZoomedSquareDialog() = default;

int ZoomedSquareDialog::selectedPodIndex() const
{
    return _selectedPodIndex;
}

bool ZoomedSquareDialog::shouldMovePod() const
{
    return _shouldMovePod;
}

bool ZoomedSquareDialog::moveAllMatchingStack() const
{
    return _moveAllMatchingStack;
}

bool ZoomedSquareDialog::actionCompleted() const
{
    return _actionCompleted;
}

void ZoomedSquareDialog::cycleSelectedPod(int delta)
{
    if (!_board || !_board->isValidCoord(_squareX - 1, _squareY - 1)) {
        return;
    }
    const OctiSquare &sq = _board->squareAt(_squareX - 1, _squareY - 1);
    int numPods = sq.getNumPods();
    if (numPods < 2) {
        return;
    }
    _selectedPodIndex = ((_selectedPodIndex + delta) % numPods + numPods) % numPods;
    if (_composer) {
        _composer->selectStackPodIndex(_squareX, _squareY, _selectedPodIndex);
    }
    update();
}

void ZoomedSquareDialog::wheelEvent(QWheelEvent *event)
{
    if (!event) return;
    int dy = event->angleDelta().y();
    if (dy > 0) {
        cycleSelectedPod(1);
    } else if (dy < 0) {
        cycleSelectedPod(-1);
    }
}

void ZoomedSquareDialog::keyPressEvent(QKeyEvent *event)
{
    if (event && event->key() == Qt::Key_Escape) {
        reject();
        return;
    }
    QDialog::keyPressEvent(event);
}

QImage ZoomedSquareDialog::renderToImage(int width, int height)
{
    QImage img(width, height, QImage::Format_ARGB32_Premultiplied);
    img.fill(Qt::transparent);
    QPainter p(&img);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::TextAntialiasing, true);

    const OctiPalette &pal = paletteForTheme(_theme);

    // 1. Background Card
    QRectF bgRect(0, 0, width, height);
    p.setPen(QPen(pal.accentBrass, 1.8));
    p.setBrush(pal.panelCardBg);
    p.drawRoundedRect(bgRect.adjusted(1, 1, -1, -1), 10.0, 10.0);

    // 2. Header
    QFont titleFont("SansSerif", 16, QFont::Bold);
    p.setFont(titleFont);
    p.setPen(pal.prongSuperGold);
    QRectF coordRect(16, 14, width - 32, 28);
    p.drawText(coordRect, Qt::AlignLeft | Qt::AlignVCenter,
               QString("Square (%1, %2)").arg(_squareX).arg(_squareY));

    bool isTurn = false;
    bool hasEnemy = false;
    bool isEmptySquare = true;
    int numPods = 0;
    if (_board && _board->isValidCoord(_squareX - 1, _squareY - 1)) {
        const OctiSquare &sq = _board->squareAt(_squareX - 1, _squareY - 1);
        numPods = sq.getNumPods();
        isEmptySquare = sq.isEmpty();
        if (_board->getGame()) {
            OctiPlayer turn = _board->getGame()->getTurn();
            isTurn = true;
            hasEnemy = sq.hasEnemyOf(turn);
        }
    }

    QFont modeFont("SansSerif", 10, QFont::Medium);
    p.setFont(modeFont);
    p.setPen(hasEnemy ? pal.legalCaptureGlow : pal.accentBrass);
    QString modeStr = hasEnemy
        ? "Close-Up Inspect Mode (Opponent Square)"
        : (isEmptySquare
               ? "Eligible Friendly Home Square"
               : "Active Turn - Pod Configuration & Move Planning");
    QRectF modeRect(16, 42, width - 32, 20);
    p.drawText(modeRect, Qt::AlignLeft | Qt::AlignVCenter, modeStr);

    // 3. Stack Selector Strip (when >= 2 pods)
    _stackPodTabRects.clear();
    _moveStackCheckRect = QRectF();
    if (numPods >= 2) {
        double tabW = std::min(80.0, (width - 150.0) / numPods);
        double tabH = 26.0;
        double startX = 120.0;
        double startY = 66.0;

        for (int i = 0; i < numPods; ++i) {
            QRectF tabR(startX + i * (tabW + 6.0), startY, tabW, tabH);
            _stackPodTabRects.push_back(tabR);

            bool sel = (i == _selectedPodIndex);
            bool claimed = (_composer && _composer->isSquarePodClaimed(_squareX, _squareY, i));
            p.setPen(QPen(sel ? pal.prongSuperGold : pal.accentBrass, sel ? 1.8 : 1.0));
            p.setBrush(sel ? pal.boardOuterFrame.darker(140) : pal.panelCardBg.darker(120));
            p.drawRoundedRect(tabR, 4.0, 4.0);

            p.setPen(claimed ? pal.textSecondary : (sel ? pal.prongSuperGold : pal.textPrimary));
            QFont tabFont("SansSerif", 9, sel ? QFont::Bold : QFont::Normal);
            p.setFont(tabFont);
            QString tabLabel = claimed
                ? QString::fromUtf8("Pod %1/%2 \xE2\x9C\x93").arg(i + 1).arg(numPods)
                : QString("Pod %1/%2").arg(i + 1).arg(numPods);
            p.drawText(tabR, Qt::AlignCenter, tabLabel);
        }

        if (!hasEnemy && isTurn) {
            _moveStackCheckRect = QRectF(120.0, 96.0, 240.0, 20.0);
            p.setPen(QPen(pal.accentBrass, 1.2));
            p.setBrush(_moveAllMatchingStack ? pal.accentBrass : Qt::NoBrush);
            p.drawRect(120.0, 98.0, 14.0, 14.0);
            if (_moveAllMatchingStack) {
                p.setPen(QPen(Qt::black, 2.0));
                p.drawLine(122.0, 105.0, 126.0, 110.0);
                p.drawLine(126.0, 110.0, 132.0, 100.0);
            }
            p.setPen(pal.textPrimary);
            QFont chkFont("SansSerif", 9);
            p.setFont(chkFont);
            p.drawText(QRectF(140.0, 96.0, 220.0, 18.0),
                       Qt::AlignLeft | Qt::AlignVCenter,
                       "Move All Matching Pods (Stack)");
        }
    }

    // 4. Large Interactive 320x320 Zoomed Pod Canvas
    _podCanvasRect = QRectF(120.0, (numPods >= 2) ? 122.0 : 76.0, 320.0, 320.0);
    p.setPen(QPen(pal.boardOuterFrame.darker(180), 1.0));
    p.setBrush(QColor(10, 14, 22, 160));
    p.drawRoundedRect(_podCanvasRect, 8.0, 8.0);

    QPointF pc = _podCanvasRect.center();
    double podR = 100.0;
    _socketRadius = 18.0;

    _socketCenters.clear();
    int rotDeg = static_cast<int>(_rotation);

    if (!isEmptySquare && _board && _board->isValidCoord(_squareX - 1, _squareY - 1)) {
        const OctiSquare &sq = _board->squareAt(_squareX - 1, _squareY - 1);
        int pIdx = std::clamp(_selectedPodIndex, 0, sq.getNumPods() - 1);

        // GUI2-09: Render other stacked pods underneath with stepped offsets and reduced opacity
        if (sq.getNumPods() >= 2) {
            int layerStep = 0;
            for (int i = 0; i < sq.getNumPods(); ++i) {
                if (i == pIdx) continue;
                layerStep++;
                const OctiPod &underPod = sq.getPodAt(i);
                QPointF offset(-layerStep * 14.0, layerStep * 14.0);
                QRectF underRect(pc.x() + offset.x() - podR,
                                 pc.y() + offset.y() - podR,
                                 podR * 2.0, podR * 2.0);
                paintOctagonalPod(p, underRect, underPod.getType(), underPod,
                                  pal, rotDeg, false, 0.28);
            }
        }

        // Active pod at exact center with full opacity
        const OctiPod &pod = sq.getPodAt(pIdx);
        QRectF pRect(pc.x() - podR, pc.y() - podR, podR * 2.0, podR * 2.0);
        paintOctagonalPod(p, pRect, pod.getType(), pod, pal, rotDeg, true, 1.0);

        // GUI2-02: Render all 8 sockets around perimeter using OctiBoardGeometry dirUnitVectorScreen
        OctiBoardGeometry zGeom(320, 320, 9, 9, _rotation);
        for (int d = DIR_N; d <= DIR_NW; ++d) {
            OctiDir dir = static_cast<OctiDir>(d);
            QPointF u = zGeom.dirUnitVectorScreen(dir);
            QPointF sc = pc + QPointF(u.x() * (podR * 0.86), u.y() * (podR * 0.86));
            _socketCenters[dir] = sc;

            bool isHovered = (dir == _hoverProngDir);
            bool isRepositionSrc = (dir == _selectedProngSocketForReposition);

            if (isRepositionSrc) {
                p.setPen(QPen(pal.prongSuperGold, 2.5));
                p.setBrush(pal.prongSuperGold.darker(160));
            } else if (isHovered) {
                p.setPen(QPen(pal.legalStepGuide, 2.2));
                p.setBrush(pal.legalStepGuide.darker(160));
            } else {
                p.setPen(QPen(pal.gridSocketShadow, 1.2));
                p.setBrush(QColor(14, 20, 30, 200));
            }
            p.drawEllipse(sc, _socketRadius, _socketRadius);

            // Socket direction letter 'a'..'h'
            QFont scFont("SansSerif", 9, QFont::Bold);
            p.setFont(scFont);
            p.setPen((isHovered || isRepositionSrc) ? pal.prongSuperGold : pal.accentBrass);
            char letter = static_cast<char>('a' + (d - DIR_N));
            p.drawText(QRectF(sc.x() - 10, sc.y() - 10, 20, 20),
                       Qt::AlignCenter, QString(letter));
        }
    } else {
        // Empty friendly home square
        p.setPen(QPen(pal.accentBrass, 1.5, Qt::DashLine));
        p.setBrush(Qt::NoBrush);
        p.drawEllipse(pc, podR * 0.8, podR * 0.8);
        QFont eFont("SansSerif", 11, QFont::Medium);
        p.setFont(eFont);
        p.setPen(pal.accentBrass);
        p.drawText(_podCanvasRect, Qt::AlignCenter, "Empty OCTI Home Square\n(Place Pod to Enter Board)");
    }

    // 5. Left Side: Prong Dispenser Tray (for friendly pods on player's turn)
    _normalProngTrayRect = QRectF();
    _superProngTrayRect = QRectF();
    bool currentPodClaimed = (!isEmptySquare && _composer &&
                              _composer->isSquarePodClaimed(_squareX, _squareY, _selectedPodIndex));
    if (! isEmptySquare && !hasEnemy && isTurn && !currentPodClaimed &&
        _board && _board->getGame()) {
        OctiPlayer turn = _board->getGame()->getTurn();
        int freeProngs = _board->getGame()->getFreeProngs(turn);
        int freeSuper = _board->getGame()->getFreeSuperProngs(turn);

        _normalProngTrayRect = QRectF(16, 140, 88, 80);
        _superProngTrayRect = QRectF(16, 235, 88, 80);

        bool npSelected = (_selectedProngType == SelectedProngType::NORMAL);
        bool spSelected = (_selectedProngType == SelectedProngType::SUPER);

        // Normal Prong Tray Card
        p.setPen(QPen(npSelected ? pal.prongSuperGold : pal.accentBrass, npSelected ? 2.0 : 1.2));
        p.setBrush(npSelected ? pal.panelCardBg : pal.panelCardBg.darker(110));
        p.drawRoundedRect(_normalProngTrayRect, 6.0, 6.0);
        QRectF npIconR(_normalProngTrayRect.center().x() - 14, _normalProngTrayRect.top() + 8, 28, 28);
        paintProngGlyph(p, npIconR, false, pal);
        QFont trayFont("SansSerif", 8, QFont::Bold);
        p.setFont(trayFont);
        p.setPen(npSelected ? pal.prongSuperGold : pal.textPrimary);
        QRectF npTxtR(_normalProngTrayRect.left(), _normalProngTrayRect.top() + 40, _normalProngTrayRect.width(), 34);
        p.drawText(npTxtR, Qt::AlignCenter, QString("Normal\n(%1 avail)").arg(freeProngs));

        // Superprong Tray Card
        p.setPen(QPen(spSelected ? pal.prongSuperGold : pal.accentBrass.darker(130), spSelected ? 2.0 : 1.2));
        p.setBrush(spSelected ? pal.panelCardBg : pal.panelCardBg.darker(110));
        p.drawRoundedRect(_superProngTrayRect, 6.0, 6.0);
        QRectF spIconR(_superProngTrayRect.center().x() - 14, _superProngTrayRect.top() + 8, 28, 28);
        paintProngGlyph(p, spIconR, true, pal);
        p.setFont(trayFont);
        p.setPen(spSelected ? pal.prongSuperGold : pal.accentBrass);
        QRectF spTxtR(_superProngTrayRect.left(), _superProngTrayRect.top() + 40, _superProngTrayRect.width(), 34);
        p.drawText(spTxtR, Qt::AlignCenter, QString("Super (!)\n(%1 avail)").arg(freeSuper));
    }

    // 6. Action Buttons at Bottom
    _movePodBtnRect = QRectF();
    _placeReserveBtnRect = QRectF();
    _liberateBtnRect = QRectF();

    OctiPlayer turn = (_board && _board->getGame()) ? _board->getGame()->getTurn() : PLAYER_A;
    const OctiSquare &sq = (_board && _board->isValidCoord(_squareX - 1, _squareY - 1))
        ? _board->squareAt(_squareX - 1, _squareY - 1)
        : _board->squareAt(0, 0);

    bool isFriendlyBase = false;
    if (_board) {
        if (_board->getNumPlayers() == 2) {
            isFriendlyBase = ((turn == PLAYER_A && sq.getType() == A_HOME_SQUARE) ||
                              (turn == PLAYER_B && sq.getType() == B_HOME_SQUARE));
        } else if (_board->getNumPlayers() == 4) {
            isFriendlyBase = (sq.getType() == static_cast<OctiSquareType>(turn));
        }
    }

    bool canShowPodPlacement = isTurn && !hasEnemy && _board &&
                               _board->getGameMode() != GAME_MODE_OFK &&
                               (isFriendlyBase || (!isEmptySquare && sq.getType() != NEUTRAL_SQUARE));

    int freePods = (_board && _board->getGame()) ? _board->getGame()->getFreePods(turn) : 0;
    int capPods = (_board && _board->getGame()) ? _board->getGame()->getCapturedPods(turn) : 0;
    bool canLiberate = (capPods > 0 && _board && _board->canLiberate(turn));

    auto drawPlaceAndLiberate = [&](const QRectF &placeRect, const QRectF &libRect, int fontSize) {
        _placeReserveBtnRect = placeRect;
        _liberateBtnRect = libRect;

        bool canPlace = (freePods > 0);
        p.setPen(QPen(canPlace ? pal.prongSuperGold : pal.accentBrass.darker(160), 1.4));
        p.setBrush(canPlace ? pal.podA.core : pal.panelCardBg.darker(130));
        p.drawRoundedRect(_placeReserveBtnRect, 6.0, 6.0);
        p.setPen(canPlace ? Qt::white : pal.textSecondary);
        p.setFont(QFont("SansSerif", fontSize, QFont::Bold));
        p.drawText(_placeReserveBtnRect, Qt::AlignCenter, "Place Reserve (+)");

        p.setPen(QPen(canLiberate ? pal.prongSuperGold : pal.accentBrass.darker(160), 1.4));
        p.setBrush(canLiberate ? pal.accentBrass.darker(140) : pal.panelCardBg.darker(130));
        p.drawRoundedRect(_liberateBtnRect, 6.0, 6.0);
        p.setPen(canLiberate ? pal.prongSuperGold : pal.textSecondary);
        p.setFont(QFont("SansSerif", fontSize, QFont::Bold));
        p.drawText(_liberateBtnRect, Qt::AlignCenter, "Liberate (*)");
    };

    if (!isEmptySquare && !hasEnemy && isTurn && !currentPodClaimed) {
        if (canShowPodPlacement) {
            _movePodBtnRect = QRectF(120, 444, 320, 32);
            drawPlaceAndLiberate(QRectF(120, 482, 155, 32), QRectF(285, 482, 155, 32), 9);
        } else {
            _movePodBtnRect = QRectF(120, 460, 320, 38);
        }

        bool canMove = false;
        if (_composer) {
            canMove = !_composer->legalDestinationsFrom(_squareX, _squareY, _selectedPodIndex).empty();
        }
        p.setPen(QPen(canMove ? pal.prongSuperGold : pal.accentBrass.darker(180), 1.4));
        p.setBrush(canMove ? pal.podA.core : pal.panelCardBg.darker(140));
        p.drawRoundedRect(_movePodBtnRect, 6.0, 6.0);
        p.setPen(canMove ? Qt::white : QColor(130, 140, 150));
        QFont btnFont("SansSerif", _movePodBtnRect.height() < 36 ? 10 : 11, QFont::Bold);
        p.setFont(btnFont);
        p.drawText(_movePodBtnRect, Qt::AlignCenter, "Move / Jump This Pod");
    } else if (isEmptySquare && canShowPodPlacement && _composer) {
        drawPlaceAndLiberate(QRectF(120, 460, 155, 38), QRectF(285, 460, 155, 38), 9);
    } else if (hasEnemy) {
        QFont roFont("SansSerif", 10, QFont::Medium);
        p.setFont(roFont);
        p.setPen(pal.textSecondary);
        p.drawText(QRectF(40, 460, width - 80, 38),
                   Qt::AlignCenter,
                   QString::fromUtf8("Read-Only Opponent Pod Inspection \xE2\x80\x94 Click Outside or [Esc] to Close"));
    } else if (currentPodClaimed) {
        QFont clFont("SansSerif", 10, QFont::Medium);
        p.setFont(clFont);
        p.setPen(pal.accentBrass);
        p.drawText(QRectF(40, 460, width - 80, 38),
                   Qt::AlignCenter,
                   QString::fromUtf8("Pod Already Moved This Turn \xE2\x80\x94 Select Another Pod Tab Above"));
    }

    // 7. Render active drag token if dragging
    if (_dragKind != ZoomDragKind::NONE) {
        QRectF dragRect(_dragCursorPos.x() - 16, _dragCursorPos.y() - 16, 32, 32);
        if (_dragKind == ZoomDragKind::ADD_NORMAL_PRONG) {
            paintProngGlyph(p, dragRect, false, pal);
        } else if (_dragKind == ZoomDragKind::ADD_SUPERPRONG) {
            paintProngGlyph(p, dragRect, true, pal);
        } else if (_dragKind == ZoomDragKind::REPOSITION_PRONG) {
            paintProngGlyph(p, dragRect, false, pal);
        }
    }

    p.end();
    return img;
}

void ZoomedSquareDialog::paintEvent(QPaintEvent *event)
{
    (void)event;
    QPainter p(this);
    p.drawImage(0, 0, renderToImage(width(), height()));
}

void ZoomedSquareDialog::mousePressEvent(QMouseEvent *event)
{
    if (!event) return;
    QPointF pos = mousePosF(event);

    // GUI2-03: Click outside to dismiss immediately
    if (!rect().contains(pos.toPoint())) {
        reject();
        return;
    }

    // 1. Check Stack tabs
    for (std::size_t i = 0; i < _stackPodTabRects.size(); ++i) {
        if (_stackPodTabRects[i].contains(pos)) {
            _selectedPodIndex = static_cast<int>(i);
            if (_composer) {
                _composer->selectStackPodIndex(_squareX, _squareY, _selectedPodIndex);
            }
            update();
            return;
        }
    }

    // 2. Check Move Stack check box
    if (!_moveStackCheckRect.isEmpty() && _moveStackCheckRect.contains(pos)) {
        _moveAllMatchingStack = !_moveAllMatchingStack;
        update();
        return;
    }

    // 3. Check Move Pod button
    if (!_movePodBtnRect.isEmpty() && _movePodBtnRect.contains(pos)) {
        if (_composer &&
            !_composer->isSquarePodClaimed(_squareX, _squareY, _selectedPodIndex) &&
            !_composer->legalDestinationsFrom(_squareX, _squareY, _selectedPodIndex).empty()) {
            _shouldMovePod = true;
            accept();
            return;
        }
    }

    // 4. Check Home square buttons
    if (!_placeReserveBtnRect.isEmpty() && _placeReserveBtnRect.contains(pos)) {
        if (_composer && _composer->stageOrCommitAddPod(_squareX, _squareY, false)) {
            _actionCompleted = true;
            accept();
            return;
        }
        return;
    }
    if (!_liberateBtnRect.isEmpty() && _liberateBtnRect.contains(pos)) {
        if (_composer && _composer->stageOrCommitAddPod(_squareX, _squareY, true)) {
            _actionCompleted = true;
            accept();
            return;
        }
        return;
    }

    // 5. Check Prong Trays (GUI2-04 toggle selection and start drag)
    if (!_normalProngTrayRect.isEmpty() && _normalProngTrayRect.contains(pos)) {
        _selectedProngType = SelectedProngType::NORMAL;
        _selectedProngSocketForReposition = DIR_UNDEF;
        _dragKind = ZoomDragKind::ADD_NORMAL_PRONG;
        _dragCursorPos = pos;
        update();
        return;
    }
    if (!_superProngTrayRect.isEmpty() && _superProngTrayRect.contains(pos)) {
        _selectedProngType = SelectedProngType::SUPER;
        _selectedProngSocketForReposition = DIR_UNDEF;
        _dragKind = ZoomDragKind::ADD_SUPERPRONG;
        _dragCursorPos = pos;
        update();
        return;
    }

    // 6. Check socket interaction (GUI2-04: both single click/tap and drag-and-drop)
    if (_board && _board->isValidCoord(_squareX - 1, _squareY - 1)) {
        const OctiSquare &sq = _board->squareAt(_squareX - 1, _squareY - 1);
        bool isTurn = (_board->getGame() && _board->getGame()->getTurn() != PLAYER_UNDEF);
        bool hasEnemy = isTurn && sq.hasEnemyOf(_board->getGame()->getTurn());

        if (!hasEnemy && isTurn && _selectedPodIndex < sq.getNumPods() &&
            (!_composer || !_composer->isSquarePodClaimed(_squareX, _squareY, _selectedPodIndex))) {
            const OctiPod &pod = sq.getPodAt(_selectedPodIndex);
            for (const auto &pair : _socketCenters) {
                OctiDir dir = pair.first;
                QPointF sc = pair.second;
                double dist = std::hypot(pos.x() - sc.x(), pos.y() - sc.y());
                if (dist <= _socketRadius * 1.5) {
                    if (pod.prongAt(dir) == NORMAL_PRONG) {
                        // Existing normal prong clicked: select for reposition / start drag
                        _selectedProngSocketForReposition = dir;
                        _dragKind = ZoomDragKind::REPOSITION_PRONG;
                        _dragSrcProngDir = dir;
                        _dragCursorPos = pos;
                        update();
                        return;
                    } else if (pod.prongAt(dir) == NO_PRONG) {
                        // Empty socket clicked
                        if (_selectedProngSocketForReposition != DIR_UNDEF &&
                            _selectedProngSocketForReposition != dir) {
                            // Reposition previously selected prong to this socket!
                            if (_composer && _composer->stageOrCommitRepositionProng(
                                    _squareX, _squareY, _selectedPodIndex,
                                    _selectedProngSocketForReposition, dir)) {
                                _actionCompleted = true;
                                accept();
                                return;
                            }
                        } else {
                            // Add new prong to this socket!
                            bool isSuper = (_selectedProngType == SelectedProngType::SUPER);
                            if (_composer && _composer->stageOrCommitAddProng(
                                    _squareX, _squareY, _selectedPodIndex, dir, isSuper)) {
                                _actionCompleted = true;
                                accept();
                                return;
                            }
                        }
                    }
                }
            }
        }
    }
}

void ZoomedSquareDialog::mouseMoveEvent(QMouseEvent *event)
{
    if (!event) return;
    QPointF pos = mousePosF(event);
    _dragCursorPos = pos;

    _hoverProngDir = DIR_UNDEF;
    for (const auto &pair : _socketCenters) {
        OctiDir dir = pair.first;
        QPointF sc = pair.second;
        double dist = std::hypot(pos.x() - sc.x(), pos.y() - sc.y());
        if (dist <= _socketRadius * 1.5) {
            _hoverProngDir = dir;
            break;
        }
    }
    update();
}

void ZoomedSquareDialog::mouseReleaseEvent(QMouseEvent *event)
{
    if (!event) {
        _dragKind = ZoomDragKind::NONE;
        return;
    }
    QPointF pos = mousePosF(event);

    if (_dragKind != ZoomDragKind::NONE && _composer) {
        OctiDir targetDir = DIR_UNDEF;
        for (const auto &pair : _socketCenters) {
            OctiDir dir = pair.first;
            QPointF sc = pair.second;
            double dist = std::hypot(pos.x() - sc.x(), pos.y() - sc.y());
            if (dist <= _socketRadius * 1.6) {
                targetDir = dir;
                break;
            }
        }

        if (targetDir != DIR_UNDEF) {
            if (_dragKind == ZoomDragKind::ADD_NORMAL_PRONG) {
                if (_composer->stageOrCommitAddProng(_squareX, _squareY, _selectedPodIndex, targetDir, false)) {
                    _actionCompleted = true;
                    accept();
                    return;
                }
            } else if (_dragKind == ZoomDragKind::ADD_SUPERPRONG) {
                if (_composer->stageOrCommitAddProng(_squareX, _squareY, _selectedPodIndex, targetDir, true)) {
                    _actionCompleted = true;
                    accept();
                    return;
                }
            } else if (_dragKind == ZoomDragKind::REPOSITION_PRONG && targetDir != _dragSrcProngDir) {
                if (_composer->stageOrCommitRepositionProng(_squareX, _squareY, _selectedPodIndex,
                                                            _dragSrcProngDir, targetDir)) {
                    _actionCompleted = true;
                    accept();
                    return;
                }
            }
        }
    }

    _dragKind = ZoomDragKind::NONE;
    _dragSrcProngDir = DIR_UNDEF;
    update();
}

/* ==================================================================== */
/* CapturePromptDialog                                                  */
/* ==================================================================== */

CapturePromptDialog::CapturePromptDialog(int jumpedX, int jumpedY,
                                         OctiPlayer jumpedPlayer,
                                         ThemeId theme,
                                         QWidget *parent)
    : QDialog(parent)
    , _jumpedX(jumpedX)
    , _jumpedY(jumpedY)
    , _jumpedPlayer(jumpedPlayer)
    , _theme(theme)
    , _captureChosen(true)
    , _miniCanvasRect(100, 65, 160, 65)
    , _podIconRect(165, 82, 30, 30)
    , _captureBtnRect(35, 150, 135, 38)
    , _leaveBtnRect(190, 150, 135, 38)
    , _draggingPod(false)
    , _dragPos()
{
    setWindowFlags(Qt::Popup | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
    setAttribute(Qt::WA_TranslucentBackground, true);
    setFixedSize(360, 210);
    setModal(true);
    setWindowTitle("Capture Jumped Pod?");
}

CapturePromptDialog::~CapturePromptDialog() = default;

bool CapturePromptDialog::captureChosen() const
{
    return _captureChosen;
}

void CapturePromptDialog::paintEvent(QPaintEvent *event)
{
    (void)event;
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::TextAntialiasing, true);

    const OctiPalette &pal = paletteForTheme(_theme);

    // Dialog background
    p.setPen(QPen(pal.accentBrass, 1.5));
    p.setBrush(pal.panelCardBg);
    p.drawRoundedRect(rect().adjusted(1, 1, -1, -1), 8.0, 8.0);

    // Title / prompt
    QFont f("SansSerif", 11, QFont::Bold);
    p.setFont(f);
    p.setPen(pal.prongSuperGold);
    QRectF promptRect(16, 16, width() - 32, 24);
    p.drawText(promptRect, Qt::AlignCenter,
               QString("Capture jumped pod at Square (%1, %2)?").arg(_jumpedX).arg(_jumpedY));

    QFont hintF("SansSerif", 9);
    p.setFont(hintF);
    p.setPen(pal.textPrimary);
    QRectF subPrompt(16, 40, width() - 32, 18);
    p.drawText(subPrompt, Qt::AlignCenter, "Click [Capture] or drag pod off the board area below:");

    // Mini-canvas board box
    p.setPen(QPen(pal.boardOuterFrame, 1.2, Qt::DashLine));
    p.setBrush(pal.panelCardBg.darker(140));
    p.drawRoundedRect(_miniCanvasRect, 6.0, 6.0);

    // Pod icon
    QRectF drawPodRect = _draggingPod ? QRectF(_dragPos.x() - 15, _dragPos.y() - 15, 30, 30) : _podIconRect;
    OctiPodType pt = playerToPodType(_jumpedPlayer);
    OctiPod pObj(pt);
    paintOctagonalPod(p, drawPodRect, pt, pObj, pal, 0, true, 1.0);

    // Buttons
    p.setPen(QPen(pal.prongSuperGold, 1.2));
    p.setBrush(pal.podA.core);
    p.drawRoundedRect(_captureBtnRect, 5.0, 5.0);
    p.setPen(Qt::white);
    p.setFont(QFont("SansSerif", 10, QFont::Bold));
    p.drawText(_captureBtnRect, Qt::AlignCenter, "Capture [x]");

    p.setPen(QPen(pal.accentBrass, 1.2));
    p.setBrush(pal.panelCardBg.darker(120));
    p.drawRoundedRect(_leaveBtnRect, 5.0, 5.0);
    p.setPen(pal.textPrimary);
    p.drawText(_leaveBtnRect, Qt::AlignCenter, "Leave on Board");
}

void CapturePromptDialog::mousePressEvent(QMouseEvent *event)
{
    if (!event) return;
    QPointF pos = mousePosF(event);

    if (!rect().contains(pos.toPoint())) {
        reject();
        return;
    }

    if (_captureBtnRect.contains(pos)) {
        _captureChosen = true;
        accept();
        return;
    }
    if (_leaveBtnRect.contains(pos)) {
        _captureChosen = false;
        accept();
        return;
    }
    if (_podIconRect.contains(pos)) {
        _draggingPod = true;
        _dragPos = pos;
        update();
        return;
    }
}

void CapturePromptDialog::mouseMoveEvent(QMouseEvent *event)
{
    if (!event) return;
    if (_draggingPod) {
        _dragPos = mousePosF(event);
        update();
    }
}

void CapturePromptDialog::mouseReleaseEvent(QMouseEvent *event)
{
    if (!event) return;
    if (_draggingPod) {
        QPointF pos = mousePosF(event);
        _draggingPod = false;
        if (!_miniCanvasRect.contains(pos)) {
            _captureChosen = true;
            accept();
            return;
        }
        update();
    }
}

/* ==================================================================== */
/* SuperProngBonusDialog                                                */
/* ==================================================================== */

SuperProngBonusDialog::SuperProngBonusDialog(ThemeId theme, QWidget *parent)
    : QDialog(parent)
    , _theme(theme)
    , _moveNowChosen(true)
    , _moveNowBtn(nullptr)
    , _skipBtn(nullptr)
{
    setWindowFlags(Qt::Popup | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
    setAttribute(Qt::WA_TranslucentBackground, true);
    setFixedSize(440, 190);
    setModal(true);
    setWindowTitle("Superprong Bonus Move Available!");

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(12);

    QLabel *titleLabel = new QLabel("8th-Prong Superprong Bonus Move Available!", this);
    titleLabel->setStyleSheet("font-size: 14px; font-weight: bold; color: #f5c542;");
    layout->addWidget(titleLabel, 0, Qt::AlignCenter);

    QLabel *msgLabel = new QLabel(
        "You have completed your pod's 8th prong with a crowned Superprong!\n"
        "You are entitled to an immediate bonus move or jump with this pod.", this);
    msgLabel->setAlignment(Qt::AlignCenter);
    msgLabel->setWordWrap(true);
    layout->addWidget(msgLabel);

    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(14);

    _moveNowBtn = new QPushButton("Move / Jump Now", this);
    _moveNowBtn->setDefault(true);
    _skipBtn = new QPushButton("Skip Bonus Move", this);

    btnLayout->addWidget(_moveNowBtn);
    btnLayout->addWidget(_skipBtn);
    layout->addLayout(btnLayout);

    QObject::connect(_moveNowBtn, &QPushButton::clicked, this, [this]() {
        _moveNowChosen = true;
        accept();
    });
    QObject::connect(_skipBtn, &QPushButton::clicked, this, [this]() {
        _moveNowChosen = false;
        accept();
    });
}

SuperProngBonusDialog::~SuperProngBonusDialog() = default;

bool SuperProngBonusDialog::moveNowChosen() const
{
    return _moveNowChosen;
}

void SuperProngBonusDialog::paintEvent(QPaintEvent *event)
{
    (void)event;
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    const OctiPalette &pal = paletteForTheme(_theme);
    p.setPen(QPen(pal.accentBrass, 1.5));
    p.setBrush(pal.panelCardBg);
    p.drawRoundedRect(rect().adjusted(1, 1, -1, -1), 10.0, 10.0);
}

/* ==================================================================== */
/* CompositePodMoveDialog                                               */
/* ==================================================================== */

CompositePodMoveDialog::CompositePodMoveDialog(int x, int y,
                                               ThemeId theme,
                                               QWidget *parent)
    : QDialog(parent)
    , _x(x)
    , _y(y)
    , _theme(theme)
    , _selectNextPodChosen(true)
    , _selectNextBtn(nullptr)
    , _confirmBtn(nullptr)
{
    setWindowFlags(Qt::Popup | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
    setAttribute(Qt::WA_TranslucentBackground, true);
    setFixedSize(440, 190);
    setModal(true);
    setWindowTitle("Composite Multi-Pod Move Available");

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(12);

    QLabel *titleLabel = new QLabel(
        QString("Another pod on Square (%1, %2) can also move this turn.").arg(_x).arg(_y), this);
    titleLabel->setStyleSheet("font-size: 13px; font-weight: bold; color: #f5c542;");
    layout->addWidget(titleLabel, 0, Qt::AlignCenter);

    QLabel *msgLabel = new QLabel(
        "In OCTI, multiple pods starting on the same square may move together\n"
        "as a single composite turn. Would you like to select another pod?", this);
    msgLabel->setAlignment(Qt::AlignCenter);
    msgLabel->setWordWrap(true);
    layout->addWidget(msgLabel);

    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(14);

    _selectNextBtn = new QPushButton("Select Next Pod", this);
    _confirmBtn = new QPushButton("Proceed to Confirm Move", this);
    _confirmBtn->setDefault(true);

    btnLayout->addWidget(_selectNextBtn);
    btnLayout->addWidget(_confirmBtn);
    layout->addLayout(btnLayout);

    QObject::connect(_selectNextBtn, &QPushButton::clicked, this, [this]() {
        _selectNextPodChosen = true;
        accept();
    });
    QObject::connect(_confirmBtn, &QPushButton::clicked, this, [this]() {
        _selectNextPodChosen = false;
        accept();
    });
}

CompositePodMoveDialog::~CompositePodMoveDialog() = default;

bool CompositePodMoveDialog::selectNextPodChosen() const
{
    return _selectNextPodChosen;
}

void CompositePodMoveDialog::paintEvent(QPaintEvent *event)
{
    (void)event;
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    const OctiPalette &pal = paletteForTheme(_theme);
    p.setPen(QPen(pal.accentBrass, 1.5));
    p.setBrush(pal.panelCardBg);
    p.drawRoundedRect(rect().adjusted(1, 1, -1, -1), 10.0, 10.0);
}

/* ==================================================================== */
/* 17. PlyConfirmDialog (GUI2-05)                                       */
/* ==================================================================== */

PlyConfirmDialog::PlyConfirmDialog(OctiMoveComposer *composer,
                                  ThemeId theme,
                                  QWidget *parent)
    : QDialog(parent)
    , _composer(composer)
    , _theme(theme)
    , _chosenAction(ResultAction::CANCEL)
    , _hasMoreMoves(false)
    , _notation()
    , _commitBtnRect()
    , _cancelBtnRect()
    , _moreMovesBtnRect()
{
    setWindowFlags(Qt::Popup | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
    setAttribute(Qt::WA_TranslucentBackground, true);
    setFixedSize(460, 150);
    setModal(true);

    if (_composer) {
        _notation = QString::fromStdString(_composer->stagedMoveNotation());
        _hasMoreMoves = _composer->canAddMoreMoves();
    }
}

PlyConfirmDialog::~PlyConfirmDialog() = default;

PlyConfirmDialog::ResultAction PlyConfirmDialog::chosenAction() const
{
    return _chosenAction;
}

bool PlyConfirmDialog::isMoreMovesVisible() const
{
    return _hasMoreMoves;
}

QString PlyConfirmDialog::stagedMoveText() const
{
    return _notation;
}

QImage PlyConfirmDialog::renderToImage(int width, int height)
{
    QImage img(width, height, QImage::Format_ARGB32_Premultiplied);
    img.fill(Qt::transparent);
    QPainter p(&img);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::TextAntialiasing, true);

    const OctiPalette &pal = paletteForTheme(_theme);

    // 1. Background Card
    QRectF bgRect(0, 0, width, height);
    p.setPen(QPen(pal.accentBrass, 1.8));
    p.setBrush(pal.panelCardBg);
    p.drawRoundedRect(bgRect.adjusted(1, 1, -1, -1), 10.0, 10.0);

    // 2. Title & Move Notation
    QFont titleFont("SansSerif", 13, QFont::Bold);
    p.setFont(titleFont);
    p.setPen(pal.prongSuperGold);
    QRectF titleRect(16, 14, width - 32, 24);
    QString titleText = _notation.isEmpty()
        ? "Confirm Staged Move"
        : QString("Confirm Staged Move: %1").arg(_notation);
    p.drawText(titleRect, Qt::AlignCenter, titleText);

    // 3. Subtitle / Instruction
    QFont subFont("SansSerif", 9);
    p.setFont(subFont);
    p.setPen(pal.textPrimary);
    QRectF subRect(16, 40, width - 32, 34);
    QString subText = "Commit turn to engine, or cancel to undo staged move.";
    if (_hasMoreMoves) {
        if (_composer && _composer->isAwaitingSuperProngBonusMove() &&
            !_composer->hasEighthProngBonusHop()) {
            subText = "Commit turn, cancel, or click More Moves for 8th-prong bonus move.";
        } else if (_composer && _composer->isGuidedMoveActive()) {
            subText = "Commit turn here, cancel, or click More Moves to continue jumping.";
        } else {
            subText = "Commit turn, cancel to undo, or move another pod in stack.";
        }
    }
    p.drawText(subRect, Qt::AlignCenter | Qt::TextWordWrap, subText);

    // 4. Buttons
    double btnH = 38.0;
    double btnY = height - btnH - 18.0;

    if (_hasMoreMoves) {
        double btnW = 126.0;
        double gap = 14.0;
        double startX = (width - (3 * btnW + 2 * gap)) * 0.5;

        _commitBtnRect = QRectF(startX, btnY, btnW, btnH);
        _cancelBtnRect = QRectF(startX + btnW + gap, btnY, btnW, btnH);
        _moreMovesBtnRect = QRectF(startX + 2 * (btnW + gap), btnY, btnW, btnH);

        // Commit Button
        p.setPen(QPen(pal.prongSuperCore, 1.2));
        p.setBrush(pal.podA.core);
        p.drawRoundedRect(_commitBtnRect, 6.0, 6.0);
        p.setPen(Qt::white);
        p.setFont(QFont("SansSerif", 10, QFont::Bold));
        p.drawText(_commitBtnRect, Qt::AlignCenter, "Commit [Enter]");

        // Cancel Button
        p.setPen(QPen(pal.accentBrass, 1.2));
        p.setBrush(pal.panelCardBg.darker(130));
        p.drawRoundedRect(_cancelBtnRect, 6.0, 6.0);
        p.setPen(pal.textPrimary);
        p.drawText(_cancelBtnRect, Qt::AlignCenter, "Cancel [Esc]");

        // More Moves Button
        p.setPen(QPen(pal.prongSuperGold, 1.2));
        p.setBrush(pal.accentBrass.darker(150));
        p.drawRoundedRect(_moreMovesBtnRect, 6.0, 6.0);
        p.setPen(pal.prongSuperGold);
        p.drawText(_moreMovesBtnRect, Qt::AlignCenter, "More Moves [M]");
    } else {
        double btnW = 150.0;
        double gap = 24.0;
        double startX = (width - (2 * btnW + gap)) * 0.5;

        _commitBtnRect = QRectF(startX, btnY, btnW, btnH);
        _cancelBtnRect = QRectF(startX + btnW + gap, btnY, btnW, btnH);
        _moreMovesBtnRect = QRectF();

        // Commit Button
        p.setPen(QPen(pal.prongSuperCore, 1.2));
        p.setBrush(pal.podA.core);
        p.drawRoundedRect(_commitBtnRect, 6.0, 6.0);
        p.setPen(Qt::white);
        p.setFont(QFont("SansSerif", 10, QFont::Bold));
        p.drawText(_commitBtnRect, Qt::AlignCenter, "Commit [Enter]");

        // Cancel Button
        p.setPen(QPen(pal.accentBrass, 1.2));
        p.setBrush(pal.panelCardBg.darker(130));
        p.drawRoundedRect(_cancelBtnRect, 6.0, 6.0);
        p.setPen(pal.textPrimary);
        p.drawText(_cancelBtnRect, Qt::AlignCenter, "Cancel [Esc]");
    }

    p.end();
    return img;
}

void PlyConfirmDialog::paintEvent(QPaintEvent *event)
{
    (void)event;
    QPainter p(this);
    p.drawImage(0, 0, renderToImage(width(), height()));
}

void PlyConfirmDialog::mousePressEvent(QMouseEvent *event)
{
    if (!event) return;
    QPointF pos = mousePosF(event);

    if (!rect().contains(pos.toPoint())) {
        _chosenAction = ResultAction::CANCEL;
        if (_composer) {
            _composer->cancelAndRollback();
        }
        reject();
        return;
    }

    if (_commitBtnRect.contains(pos)) {
        _chosenAction = ResultAction::COMMIT;
        if (_composer) {
            if (_composer->isGuidedMoveActive()) {
                _composer->finishGuidedMoveHere();
            }
            _composer->commitStagedMove();
        }
        accept();
        return;
    }

    if (_cancelBtnRect.contains(pos)) {
        _chosenAction = ResultAction::CANCEL;
        if (_composer) {
            _composer->cancelAndRollback();
        }
        reject();
        return;
    }

    if (_hasMoreMoves && _moreMovesBtnRect.contains(pos)) {
        _chosenAction = ResultAction::MORE_MOVES;
        accept();
        return;
    }
}

void PlyConfirmDialog::keyPressEvent(QKeyEvent *event)
{
    if (!event) return;

    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        _chosenAction = ResultAction::COMMIT;
        if (_composer) {
            if (_composer->isGuidedMoveActive()) {
                _composer->finishGuidedMoveHere();
            }
            _composer->commitStagedMove();
        }
        accept();
        return;
    }

    if (event->key() == Qt::Key_Escape) {
        _chosenAction = ResultAction::CANCEL;
        if (_composer) {
            _composer->cancelAndRollback();
        }
        reject();
        return;
    }

    if (_hasMoreMoves && event->key() == Qt::Key_M) {
        _chosenAction = ResultAction::MORE_MOVES;
        accept();
        return;
    }

    QDialog::keyPressEvent(event);
}

/* ==================================================================== */
/* createMenuIcon                                                       */
/* ==================================================================== */

QIcon createMenuIcon(MenuIconKind kind, ThemeId theme)
{
    const OctiPalette &pal = paletteForTheme(theme);
    QImage img(32, 32, QImage::Format_ARGB32_Premultiplied);
    img.fill(Qt::transparent);
    QPainter p(&img);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::TextAntialiasing, true);

    switch (kind) {
    case MenuIconKind::NEW_GAME: {
        p.setPen(QPen(pal.accentBrass, 1.5));
        p.setBrush(pal.podA.core);
        QRectF r(4, 4, 24, 24);
        p.drawRoundedRect(r, 6, 6);
        p.setPen(QPen(pal.prongSuperGold, 2.0));
        p.drawLine(16, 9, 16, 23);
        p.drawLine(9, 16, 23, 16);
        break;
    }
    case MenuIconKind::BOARD_VIEW: {
        p.setPen(QPen(pal.accentBrass, 1.2));
        for (int row = 0; row < 3; ++row) {
            for (int col = 0; col < 3; ++col) {
                p.setBrush(((row + col) % 2 == 0) ? pal.gridCellLight : pal.gridCellDark);
                p.drawRect(5 + col * 7, 5 + row * 7, 7, 7);
            }
        }
        break;
    }
    case MenuIconKind::LOBBY_VIEW: {
        p.setPen(QPen(pal.accentBrass, 1.5));
        p.setBrush(pal.panelCardBg);
        p.drawEllipse(7, 6, 8, 8);
        p.drawArc(4, 15, 14, 12, 0, 180 * 16);
        p.drawEllipse(17, 10, 8, 8);
        p.drawArc(14, 19, 14, 12, 0, 180 * 16);
        break;
    }
    case MenuIconKind::OFFER_DRAW: {
        p.setPen(QPen(pal.accentBrass, 1.5));
        p.drawLine(16, 6, 16, 24);
        p.drawLine(8, 12, 24, 12);
        p.drawLine(8, 12, 6, 18);
        p.drawLine(24, 12, 26, 18);
        p.setBrush(pal.accentBrass);
        p.drawChord(3, 16, 8, 6, 180 * 16, 180 * 16);
        p.drawChord(21, 16, 8, 6, 180 * 16, 180 * 16);
        break;
    }
    case MenuIconKind::PAUSE_RESUME: {
        p.setPen(QPen(pal.prongSuperGold, 1.2));
        p.setBrush(pal.prongSuperGold);
        p.drawRoundedRect(7, 7, 6, 18, 2, 2);
        p.drawRoundedRect(19, 7, 6, 18, 2, 2);
        break;
    }
    case MenuIconKind::UNDO_MOVE: {
        p.setPen(QPen(pal.accentBrass, 2.2));
        p.setBrush(Qt::NoBrush);
        p.drawArc(6, 6, 20, 20, 45 * 16, 270 * 16);
        QPolygonF head;
        head << QPointF(5, 12) << QPointF(11, 8) << QPointF(11, 16);
        p.setBrush(pal.accentBrass);
        p.drawPolygon(head);
        break;
    }
    case MenuIconKind::CLAIM_FORFEIT: {
        p.setPen(QPen(pal.accentBrass, 1.6));
        p.setBrush(pal.panelCardBg);
        p.drawEllipse(5, 7, 22, 22);
        p.drawLine(16, 4, 16, 7);
        p.setPen(QPen(pal.prongSuperGold, 2.0));
        p.drawLine(16, 11, 16, 17);
        p.drawPoint(16, 22);
        break;
    }
    case MenuIconKind::RESIGN: {
        p.setPen(QPen(pal.accentBrass, 1.8));
        p.drawLine(8, 5, 8, 27);
        p.setPen(QPen(Qt::white, 1.2));
        p.setBrush(Qt::white);
        QPolygonF flag;
        flag << QPointF(9, 6) << QPointF(24, 11) << QPointF(9, 17);
        p.drawPolygon(flag);
        break;
    }
    case MenuIconKind::QUIT: {
        p.setPen(QPen(pal.legalCaptureGlow, 2.0));
        p.setBrush(Qt::NoBrush);
        p.drawArc(6, 8, 20, 20, -50 * 16, 280 * 16);
        p.drawLine(16, 5, 16, 15);
        break;
    }
    case MenuIconKind::ROTATE_BOARD: {
        p.setPen(QPen(pal.accentBrass, 2.0));
        p.setBrush(Qt::NoBrush);
        p.drawArc(6, 6, 20, 20, 30 * 16, 240 * 16);
        QPolygonF arr;
        arr << QPointF(23, 14) << QPointF(27, 8) << QPointF(20, 8);
        p.setBrush(pal.accentBrass);
        p.drawPolygon(arr);
        break;
    }
    case MenuIconKind::SWITCH_THEME: {
        p.setPen(QPen(pal.accentBrass, 1.6));
        p.setBrush(pal.panelCardBg);
        p.drawEllipse(5, 5, 22, 22);
        p.setBrush(pal.accentBrass);
        p.drawChord(5, 5, 22, 22, 90 * 16, 180 * 16);
        break;
    }
    case MenuIconKind::TOGGLE_SFX: {
        p.setPen(QPen(pal.accentBrass, 1.4));
        p.setBrush(pal.accentBrass);
        QPolygonF speaker;
        speaker << QPointF(6, 11) << QPointF(11, 11) << QPointF(16, 6)
                << QPointF(16, 26) << QPointF(11, 21) << QPointF(6, 21);
        p.drawPolygon(speaker);
        p.setBrush(Qt::NoBrush);
        p.drawArc(16, 10, 8, 12, -45 * 16, 90 * 16);
        p.drawArc(18, 6, 10, 20, -45 * 16, 90 * 16);
        break;
    }
    case MenuIconKind::VCR_FIRST: {
        p.setPen(QPen(pal.accentBrass, 1.8));
        p.drawLine(7, 8, 7, 24);
        p.setBrush(pal.accentBrass);
        QPolygonF tri;
        tri << QPointF(11, 16) << QPointF(24, 8) << QPointF(24, 24);
        p.drawPolygon(tri);
        break;
    }
    case MenuIconKind::VCR_PREV: {
        p.setPen(QPen(pal.accentBrass, 1.5));
        p.setBrush(pal.accentBrass);
        QPolygonF tri;
        tri << QPointF(8, 16) << QPointF(22, 8) << QPointF(22, 24);
        p.drawPolygon(tri);
        break;
    }
    case MenuIconKind::VCR_NEXT: {
        p.setPen(QPen(pal.accentBrass, 1.5));
        p.setBrush(pal.accentBrass);
        QPolygonF tri;
        tri << QPointF(24, 16) << QPointF(10, 8) << QPointF(10, 24);
        p.drawPolygon(tri);
        break;
    }
    case MenuIconKind::VCR_LAST: {
        p.setPen(QPen(pal.accentBrass, 1.8));
        p.drawLine(25, 8, 25, 24);
        p.setBrush(pal.accentBrass);
        QPolygonF tri;
        tri << QPointF(21, 16) << QPointF(8, 8) << QPointF(8, 24);
        p.drawPolygon(tri);
        break;
    }
    case MenuIconKind::SIGN_IN: {
        p.setPen(QPen(pal.accentBrass, 1.6));
        p.setBrush(Qt::NoBrush);
        p.drawEllipse(6, 7, 10, 10);
        p.drawLine(15, 15, 26, 24);
        p.drawLine(21, 20, 23, 18);
        p.drawLine(24, 22, 26, 20);
        break;
    }
    case MenuIconKind::ACCOUNT_LINK: {
        p.setPen(QPen(pal.accentBrass, 1.8));
        p.setBrush(Qt::NoBrush);
        p.drawRoundedRect(6, 10, 12, 7, 3, 3);
        p.drawRoundedRect(14, 15, 12, 7, 3, 3);
        break;
    }
    case MenuIconKind::GAME_ARCHIVE: {
        p.setPen(QPen(pal.accentBrass, 1.5));
        p.setBrush(pal.panelCardBg);
        p.drawRect(7, 5, 18, 22);
        p.drawLine(11, 10, 21, 10);
        p.drawLine(11, 15, 21, 15);
        p.drawLine(11, 20, 18, 20);
        break;
    }
    case MenuIconKind::LEADERBOARD: {
        p.setPen(QPen(pal.accentBrass, 1.4));
        p.setBrush(pal.accentBrass);
        p.drawRect(5, 15, 7, 12);
        p.drawRect(12, 8, 8, 19);
        p.drawRect(20, 18, 7, 9);
        break;
    }
    case MenuIconKind::MODERATION: {
        p.setPen(QPen(pal.accentBrass, 1.6));
        p.setBrush(pal.panelCardBg);
        QPolygonF shield;
        shield << QPointF(16, 5) << QPointF(25, 9) << QPointF(25, 17)
               << QPointF(16, 27) << QPointF(7, 17) << QPointF(7, 9);
        p.drawPolygon(shield);
        break;
    }
    case MenuIconKind::CHECK_UPDATE: {
        p.setPen(QPen(pal.prongSuperGold, 1.6));
        p.drawLine(16, 12, 16, 24);
        QPolygonF arr;
        arr << QPointF(16, 26) << QPointF(12, 21) << QPointF(20, 21);
        p.setBrush(pal.prongSuperGold);
        p.drawPolygon(arr);
        p.setBrush(Qt::NoBrush);
        p.drawArc(7, 8, 18, 12, 0, 180 * 16);
        break;
    }
    case MenuIconKind::SHOW_SPLASH: {
        p.setPen(QPen(pal.prongSuperGold, 1.8));
        p.setBrush(pal.podA.core);
        QRectF r(6, 6, 20, 20);
        p.drawRoundedRect(r, 5, 5);
        p.setPen(QPen(pal.prongSuperGold, 1.5));
        p.drawLine(16, 8, 16, 24);
        p.drawLine(8, 16, 24, 16);
        p.drawLine(10, 10, 22, 22);
        p.drawLine(22, 10, 10, 22);
        break;
    }
    }

    p.end();
    return QIcon(QPixmap::fromImage(img));
}

} // namespace octi

/*
 * Local variables:
 * mode: C++
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
