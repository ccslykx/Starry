#include <QLineEdit>
#include <QVBoxLayout>
#include <QFileDialog>
#include <QCloseEvent>

#include "SConfig.h"
#include "SSettings.h"
#include "SPluginEditor.h"
#include "SButton.h"
#include "utils.h"

SPluginEditor* SPluginEditor::m_instance = nullptr;

SPluginEditor* SPluginEditor::editor(QWidget *parent)
{
    SDEBUG
    if (!m_instance)
    {
        m_instance = new SPluginEditor(parent);
    }
    return m_instance;
}

void SPluginEditor::edit(SPluginInfo *info)
{
    SDEBUG
    if (!info)
    {
        return;
    }
    m_editingInfo = info;

    m_iconContainor->setPixmap(info->icon);
    m_nameEdit->setText(info->name);
    m_tipEdit->setText(info->tip);
    m_scriptEdit->setText(info->script);
    m_cButton->setVisible(false);
    m_eButton->setVisible(true);

    this->setWindowTitle(tr("Edit") + ' ' + info->name);
    this->show();
}

void SPluginEditor::create()
{
    SDEBUG
    initialize();
    this->setWindowTitle(tr("Create New Plugin"));
    this->show();
}

/* private functions */

SPluginEditor::SPluginEditor(QWidget *parent)
{
    SDEBUG
    this->setParent(parent);
    initGui();
}

void SPluginEditor::initGui()
{
    SDEBUG
    if (!m_iconLabel)   m_iconLabel = new QLabel(tr("Icon"), this);
    if (!m_nameLabel)   m_nameLabel = new QLabel(tr("Name"), this);
    if (!m_tipLabel)    m_tipLabel = new QLabel(tr("Tip"), this);
    if (!m_scriptLabel) m_scriptLabel = new QLabel(tr("Script"), this);
    if (!m_eButton)     m_eButton = new SButton(tr("Save"), this);
    if (!m_cButton)     m_cButton = new SButton(tr("Create"), this);

    if (!m_iconContainor)   m_iconContainor = new SButton("", this);
    if (!m_nameEdit)        m_nameEdit = new QLineEdit(this);
    if (!m_tipEdit)         m_tipEdit = new QLineEdit(this);
    if (!m_scriptEdit)      m_scriptEdit = new QLineEdit(this);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(m_iconLabel);
    layout->addWidget(m_iconContainor);
    layout->addWidget(m_nameLabel);
    layout->addWidget(m_nameEdit);
    layout->addWidget(m_tipLabel);
    layout->addWidget(m_tipEdit);
    layout->addWidget(m_scriptLabel);
    layout->addWidget(m_scriptEdit);
    layout->addWidget(m_eButton);
    layout->addWidget(m_cButton);

    this->setLayout(layout);

    QObject::connect(m_iconContainor, &SButton::clicked, [this] () {
        QString tmp = QFileDialog::getOpenFileName(this, tr("Select icon from file"), QDir::homePath(), tr("Images (*.bmp *.jpeg *jpg *png)"));
        if (tmp.isEmpty()) // Canceled
        {
            return;
        }
        this->m_iconPath = tmp;
        this->m_icon = QPixmap(this->m_iconPath);
        this->m_iconContainor->setPixmap(m_icon);
    });
    QObject::connect(m_eButton, &SButton::clicked, this, [this]() {
        SPluginInfo *info = this->m_editingInfo;
        if (info->name != this->m_nameEdit->text())
        {
            info->name = this->m_nameEdit->text();
            emit info->nameChanged(info);
        }
        if (!m_iconPath.isEmpty())
        {
            info->icon = this->m_icon;
            emit info->iconChanged(info);
        }

        info->script = this->m_scriptEdit->text();
        info->tip = this->m_tipEdit->text();
        
        emit info->edited(info);
        this->close();
    });
    QObject::connect(m_cButton, &SButton::clicked, this, [this]() {
        SPluginInfo *info = new SPluginInfo(this->m_nameEdit->text(), 
            this->m_scriptEdit->text(), m_icon, 0, this->m_tipEdit->text(), true);
        emit created(info);
        this->close();
    });
}

void SPluginEditor::initialize()
{
    QPixmap defaultIcon(":/default_icon.png");
    m_iconContainor->setPixmap(std::move(defaultIcon));
    m_nameEdit->setText("");
    m_tipEdit->setText("");
    m_scriptEdit->setText("");
    m_iconPath = QString();

    m_eButton->setVisible(false);
    m_cButton->setVisible(true);

    m_nameEdit->setFocus();
}

void SPluginEditor::closeEvent(QCloseEvent *ev)
{
    initialize();
    ev->accept();
}