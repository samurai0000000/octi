/*
 * OctiDialogs.cxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#include <algorithm>
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
#include "OctiDialogs.hxx"

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

    _serverBadgeLabel = new QLabel(
        QString("Server: %1:%2 (Read-Only Canonical Endpoint)")
            .arg(QString::fromUtf8(OCTI_SERVER_HOST))
            .arg(OCTI_SERVER_PORT),
        this);
    _serverBadgeLabel->setObjectName("canonicalServerBadge");
    _serverBadgeLabel->setStyleSheet(
        "padding: 6px 10px; border-radius: 6px; font-weight: bold;");
    root->addWidget(_serverBadgeLabel);

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

    _statusLabel = new QLabel(
        "Ready to authenticate with trumpet.selfso.com:6874.", this);
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
      _winnerLabel(nullptr),
      _terminationLabel(nullptr),
      _movesLabel(nullptr),
      _exportBtn(nullptr),
      _rematchBtn(nullptr),
      _moveCount(moveCount),
      _transcript(octiTranscript),
      _rematchRequested(false)
{
    setWindowTitle("OCTI — Game Over");
    setMinimumSize(380, 240);

    QVBoxLayout *root = new QVBoxLayout(this);

    _winnerLabel = new QLabel(QString::fromStdString(winnerBanner), this);
    _winnerLabel->setStyleSheet("font-size: 18px; font-weight: bold;");
    root->addWidget(_winnerLabel);

    _terminationLabel = new QLabel(
        QString("Termination: %1").arg(QString::fromStdString(terminationReason)),
        this);
    root->addWidget(_terminationLabel);

    _movesLabel = new QLabel(
        QString("Total Plies / Moves: %1").arg(moveCount), this);
    root->addWidget(_movesLabel);

    QHBoxLayout *btns = new QHBoxLayout();
    _exportBtn = new QPushButton("Export .octi", this);
    _rematchBtn = new QPushButton("Request / Accept Rematch", this);
    QPushButton *closeBtn = new QPushButton("Close", this);
    btns->addWidget(_exportBtn);
    btns->addWidget(_rematchBtn);
    btns->addWidget(closeBtn);
    root->addLayout(btns);

    QObject::connect(_rematchBtn, &QPushButton::clicked, [this]() {
        triggerRematch();
    });
    QObject::connect(closeBtn, &QPushButton::clicked, [this]() {
        accept();
    });

    setTheme(_theme);
}

GameOverDialog::~GameOverDialog() = default;

void GameOverDialog::setTheme(ThemeId theme)
{
    _theme = theme;
    setStyleSheet(buildAppStyleSheet(_theme));
}

ThemeId GameOverDialog::theme() const
{
    return _theme;
}

QString GameOverDialog::winnerBannerText() const
{
    return _winnerLabel ? _winnerLabel->text() : QString();
}

QString GameOverDialog::terminationText() const
{
    return _terminationLabel ? _terminationLabel->text() : QString();
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
    if (_onRematchCallback) {
        _onRematchCallback();
    }
    accept();
}

bool GameOverDialog::wasRematchRequested() const
{
    return _rematchRequested;
}

void GameOverDialog::setOnRematchCallback(std::function<void()> cb)
{
    _onRematchCallback = std::move(cb);
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
