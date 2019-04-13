// Copyright (c) 2019 Softwarebuero Krekeler

// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "documentlist.h"
#include "ui_documentlist.h"

#include "sync.h"
#include "clientmodel.h"
#include "walletmodel.h"
#include "wallet/wallet.h"
#include "init.h"
#include "amount.h"
#include "bitcoinunits.h"
#include "optionsmodel.h"
#include "util.h"
#include "guiutil.h"
#include "rpcconsole.h"
#include "transactionrecord.h"

#include <QCryptographicHash>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QInputDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QSettings>
#include <QStringList>
#include <QUuid>

/** local functions
*/
namespace {

const QString descFileExt = ".desc";
const double minDocRevFee = 0.1; // DMS

/** frequent terms */
QString trDocument;
QString trFileHash;
QString trGUID;
QString trAttrHash;
QString trBlockchain;
QString trDocRev;
QString trExecute;
QString trTransID;
QString trTotalAmount;
QString trStored;

QString getTextHash(const QString text)
{
    QCryptographicHash crypto(QCryptographicHash::Md5);
    crypto.addData(text.toUtf8());
    QByteArray hash = crypto.result();

    return QString::fromLatin1(hash.toHex()).toUpper();
}

QString getFileHash(const QString fileName)
{
    QCryptographicHash crypto(QCryptographicHash::Md5);
    QFile file(fileName);
    file.open(QFile::ReadOnly);
    while(!file.atEnd()){
      crypto.addData(file.read(8192));
    }
    QByteArray hash = crypto.result();

    return QString::fromLatin1(hash.toHex()).toUpper();
}

QString compressGuid(QString GUID)
{
  return GUID.remove("{").remove("}").remove("-");
}
} // namespace

/** Document
*/

Document::Document(const QString docFileName)
{
    filename = docFileName;
    minConfirms = TransactionRecord::RecommendedNumConfirmations;

    QFileInfo fileInfo(filename);
    descfilename = fileInfo.path() + "/desc/" + fileInfo.fileName() + descFileExt;

    // create the description file if not exists
    if (!QFile::exists(descfilename)) {
        // attribute hash: we hash the filename and the size in byte
        // in a DMS you can hash several relevant attributes like numer and receipt date
        QString attr = fileInfo.fileName() + " " +  QString::number(fileInfo.size());

        QSettings descFile(descfilename, QSettings::IniFormat);
        descFile.beginGroup("docfile");
        descFile.setValue("GUID", QUuid::createUuid().toString().toUpper());
        descFile.setValue("name", fileInfo.fileName());
        descFile.setValue("filesize", fileInfo.size());
        descFile.setValue("filehash", getFileHash(docFileName));
        descFile.setValue("attrhash", getTextHash(attr));
        descFile.endGroup();

        descFile.sync();
    }
    // load local desc file
    loadDescription();
}

Document::~Document()
{
}

void Document::loadDescription()
{
    QSettings descFile(descfilename, QSettings::IniFormat);

    descFile.beginGroup("docfile");
    name = descFile.value("name", "").toString();
    guid = descFile.value("GUID", "").toString();
    filesize = descFile.value("filesize", "").toInt();
    filehash = descFile.value("filehash", "").toString();
    attrhash = descFile.value("attrhash", "").toString();
    descFile.endGroup();

    descFile.beginGroup("blockchain");
    txid = descFile.value("txid", "").toString();
    savetime = descFile.value("systemtime", "").toDateTime();

    descFile.endGroup();
}

/** we are using RPC functions to create the transaction
 * developer can use this as template in there document management system
 */
QString Document::documentRevision()
{
    std::string result;
    std::string filtered;
    QString txasm;
    QString bcguid;
    QString bcfilehash;
    QString bcattrhash;

    QString revOk    = "<h2 style=\"color:green\">" + tr("Revision successful") + "</h2>";
    QString revHint  = "<h2 style=\"color:green\">" + tr("Revision with notes") + "</h2>";
    QString revNoConf= "<h2 style=\"color:red\">"   + tr("Revision is not possible yet") + "</h2>";
    QString revError = "<h2 style=\"color:red\">"   + tr("Revision failed") + "</h2>";

    if (txid.isEmpty())
        return revError + "<p>" + tr("No Transaction stored.") + "</p>";
    
    if (filehash != getFileHash(filename))
        return revError + "<p>" 
            + tr("The current file does not match the locally saved hash. This file has been modified.") + "</p>";

    try {
        // 1. get transaction
        // RPC: Post('{"jsonrpc": "1.0", "id":"DMSExposed", "method": "getrawtransaction", "params": ["%1",true] }',
        RPCConsole::RPCExecuteCommandLine(result, "getrawtransaction " + txid.toStdString() + " true", &filtered);

        QJsonDocument jdoc = QJsonDocument::fromJson(QByteArray::fromStdString(result));
        QJsonObject jobj = jdoc.object();

        int confirmations = jobj.value("confirmations").toInt();
        QDateTime time = QDateTime::fromTime_t(jobj.value("time").toInt());
        QDateTime blocktime = QDateTime::fromTime_t(jobj.value("blocktime").toInt());
        int height = jobj.value("height").toInt();
        QString blockhash = jobj.value("blockhash").toString();

        bool found = false;
        QJsonArray jary = jobj.value("vout").toArray();
        BOOST_FOREACH (const QJsonValue & jarval, jary) {
            QJsonObject jvout = jarval.toObject();
            // {"value": 0.00000000,"valueSat": 0,"n": 1,"scriptPubKey": {"asm": "OP_RETURN 48616c6c6f2057656c74","hex": "6a0a48616c6c6f2057656c74","type": "nulldata"}}
            jvout = jvout.value("scriptPubKey").toObject();
            if (jvout.value("type").toString() == "nulldata") {
                txasm = jvout.value("asm").toString();
                if (txasm.toUpper().startsWith("OP_RETURN 444D2400010002")) {
                    txasm.remove(0, 10);
                    bcguid = txasm.mid(14, 32).toUpper();
                    bcfilehash = txasm.mid(46, 32).toUpper();
                    bcattrhash = txasm.mid(78, 32).toUpper();
                    found = true;
                    break;
                }
            }
        }
        if (!found)
            return revError + "<p>" + tr("Transaction not found.") + "</p>";

        // compare document with data from blockchain
        QString revlog = "";
        QString deviation  = tr("current value \"%1\" differs from value \"%2\" stored in blockchain.");
        QString accordance = tr("matches the blockchain value.");

        if (confirmations < minConfirms) {
            revlog.append(revNoConf);
            revlog.append("<p>" + tr("There are not enough blockchain confirmations available for revision. Please wait a while.") + "</p>");
        }
        else if (compressGuid(guid) == bcguid && attrhash == bcattrhash && filehash == bcfilehash)
            revlog.append(revOk);
        else if (filehash == bcfilehash)
            revlog.append(revHint);
        else
            revlog.append(revError);

        revlog.append("<p>");
        if (compressGuid(guid) == bcguid)
            revlog.append(QString("<b>%1</b>: <span style=\"color:green\">%2</span><br>").arg(trGUID, accordance));
        else
            revlog.append(QString("<b>%1</b>: <span style=\"color:red\">%2</span><br>")
                         .arg(trGUID, deviation.arg(compressGuid(guid), bcguid)));
        if (attrhash == bcattrhash)
            revlog.append(QString("<b>%1</b>: <span style=\"color:green\">%2</span><br>").arg(trAttrHash, accordance));
        else
            revlog.append(QString("<b>%1</b>: <span style=\"color:red\">%2</span><br>")
                         .arg(trAttrHash, deviation.arg(compressGuid(guid), bcguid)));
        if (filehash == bcfilehash) {
            revlog.append(QString("<b>%1</b>: <span style=\"color:green\">%2</span>").arg(trFileHash, accordance));
            if (confirmations >= minConfirms)
                revlog.append("</p><p>" + tr("The blockchain confirms that %1this document file%2 exists at least since %3 and has not been modified.")
                                            .arg("<a href=\"open\">", "</a>", GUIUtil::dateTimeStr(blocktime)));
        }
        else
            revlog.append(QString("<b>%1</b>: <span style=\"color:red\">%2</span>")
                         .arg(trFileHash, deviation.arg(compressGuid(guid), bcguid)));

        revlog.append("</p><p><b>" + tr("Blockchain confirmations") 
            + "</b>: " + QString::number(confirmations) 
            + ((confirmations < minConfirms) ? ("/" + QString::number(minConfirms)) : "") + "</p>");

        return revlog;

    } catch (const std::exception& e) {
        return revError + "<p>" + QString::fromStdString(e.what()) + "</p>";
    } catch (...) {
        return  revError + "<p>" + tr("Unknown error.") + "</p>";
    }
}

QString Document::getInformationHtml()
{
    return QString("<h1>%1</h1><h2>%9</h2>"
        "<p><a href=\"open\">%2</a> (%3 byte)</p>"
        "<p><b>%10</b><br>%4</p>"
        "<p><b>%11</b><br>%5<br>"
        "<b>%12</b><br>%6</p>"
        "<h2>%13</h2>"
        "<p><b>%14</b><br>%7</p>"
        "<p><b>%15</b><br>%8 (local system time)</p>"
        ).arg(
        name, filename, QString::number(filesize), guid, filehash, attrhash, txid,
        GUIUtil::dateTimeStr(savetime)  // %8
        ).arg(
        trDocument, trGUID, trFileHash, trAttrHash, trBlockchain, trTransID, trStored);
}

/** we are using RPC functions to create the transaction
 * developer can use this as template in there document management system
 */
QString Document::writeToBlockchain()
{
    std::string result;
    std::string filtered;
    std::string changetx = "";
    double tmpamount;
    double usedamount = -1;
    double mininput = INT_MAX; 
    QString txid = "";
    int vout = -1; 

    try {
        // 1. read description file "../data/documents/desc/filename.desc"
        QSettings descFile(descfilename, QSettings::IniFormat);
        descFile.beginGroup("blockchain");
        if (descFile.value("txid", "").toString() != "") {
            QMessageBox::critical(NULL, tr("Rejected"), 
                tr("This file information has already been stored in Documentchain."));
            return "";
        }
        descFile.endGroup();

        descFile.beginGroup("docfile");
        QString comprguid = compressGuid(descFile.value("GUID", "").toString());
        QString filehash = descFile.value("filehash", "").toString();
        QString attrhash = descFile.value("attrhash", "").toString();
        descFile.endGroup();
        if ( comprguid.length() != 32 || filehash.length() != 32 || attrhash.length() != 32 ) {
            QMessageBox::critical(NULL, "Desc File", tr("Invalid document description."));
            return "";
        }

        // 2. searching the lowest input to pay the fee
        // RPC: Post('{"jsonrpc":"1.0","id":"YourAppName","method":"listunspent"}');
        RPCConsole::RPCExecuteCommandLine(result, "listunspent", &filtered);
        QJsonDocument jdoc = QJsonDocument::fromJson(QByteArray::fromStdString(result));
        QJsonArray jary = jdoc.array();
        BOOST_FOREACH (const QJsonValue & jarval, jary) {
            QJsonObject jobj = jarval.toObject();
            tmpamount = jobj.value("amount").toDouble();
            if(jobj.value("spendable").toBool()
            && jobj.value("confirmations").toInt() >= TransactionRecord::RecommendedNumConfirmations
            && tmpamount <= 55  // max input is just a safety value to avoid loss of change
            && tmpamount >= minDocRevFee
            && tmpamount <  mininput) {
                txid = jobj.value("txid").toString();
                vout = jobj.value("vout").toInt();
                usedamount = tmpamount;
                mininput = tmpamount;
            }
        }
        if (txid.isEmpty()) {  // TODO: remove the 55 DMS limit above
            QMessageBox::critical(NULL, tr("Input"), 
                   "No matching credit (input) found. At least one input with a credit "
                   "between 0.1 and 55 coins and 6 confirmation required.");
            return "";
        }

        // 3. calc change amount and get address
        // RPC: Post('{"jsonrpc":"1.0","id":"YourAppName","method":"getrawchangeaddress"}');
        double change = usedamount - minDocRevFee;
        if (change > minDocRevFee / 100) {  // lower change goes to miner
            RPCConsole::RPCExecuteCommandLine(result, "getrawchangeaddress", &filtered);
            changetx = strprintf("\\\"%s\\\":%f, ", result, change);
        }

        // 4. format document revision hex data 
        //  0..5  : "444D24" = 'DM$', the magic chars for document revision
        //  6..9  : "0001"   = blockchain data version
        // 10..13 : "04D2"   = app-defined typ/version (OM DMS uses "0001", DMS Core "0002", use another ID)
        // 14..45 : document GUID without {} and -
        // 46..77 : file hash
        // 78..109: attribute hash
        // 110..  : (optional) encoded document attributes like number, name, receiptdate
        std::string docrevdata = "444D2400010002" 
                               + comprguid.toStdString()
                               + filehash.toStdString()
                               + attrhash.toStdString();

        // 5. create raw transaction
        // RPC: Post('{"jsonrpc":"1.0","id":"YourAppName","method":"createrawtransaction","params":'
        //         + '[[{"txid":"%1","vout":%2}], {%3"data":"%4"}]}');
        //         %1=txid, %2=vout, %3=changetx, 4=docrevdata
        std::string command = strprintf(
            "createrawtransaction \"[{\\\"txid\\\":\\\"%s\\\",\\\"vout\\\":%d}]\" \"{%s\\\"data\\\":\\\"%s\\\"}\"",
            txid.toStdString(), vout, changetx, docrevdata);
        RPCConsole::RPCExecuteCommandLine(result, command, &filtered);

        // 6. sign raw transaction
        // RPC: Post('{"jsonrpc":"1.0","id":"YourAppName","method":"signrawtransaction","params":["%1"]}');
        command = "signrawtransaction " + result;
        RPCConsole::RPCExecuteCommandLine(result, command, &filtered);
        jdoc = QJsonDocument::fromJson(QByteArray::fromStdString(result));
        if (!jdoc.object().value("complete").toBool()) {
            QMessageBox::critical(NULL, tr("Signing"), tr("Could not sign transaction."));
            return "";
        }
        QString signedtrans = jdoc.object().value("hex").toString();

        // 7. send raw transaction
        // RPC: Post('{"jsonrpc":"1.0","id":"YourAppName","method":"sendrawtransaction","params":["%1"]});
        // this returns the transaction id, save it together with the document in your document archive
        command = "sendrawtransaction " + signedtrans.toStdString();
        RPCConsole::RPCExecuteCommandLine(result, command, &filtered);
        txid = QString::fromStdString(result);

        // 8. store transaction id together with the document in description file
        descFile.beginGroup("blockchain");
        descFile.setValue("txid", txid);
        descFile.setValue("systemtime", QDateTime::currentDateTime());
        descFile.endGroup();
        descFile.sync();

        // finished
        return txid;

    } catch (const std::exception& e) {
        QMessageBox::critical(NULL, tr("RPC Error"), QString::fromStdString(e.what()));
        return "";
    } catch (...) {
        QMessageBox::critical(NULL, tr("RPC Error"), tr("Unknown error."));
        return "";
    }
}

/** DocumentList
*/

DocumentList::DocumentList(const PlatformStyle *_platformStyle, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::DocumentList),
    clientModel(0),
    walletModel(0),
    platformStyle(_platformStyle)
{
    ui->setupUi(this);

    trDocument = tr("Document");
    trFileHash = tr("File hash");
    trGUID = tr("GUID");
    trAttrHash = tr("Attribute hash");
    trBlockchain = tr("Blockchain");
    trDocRev = tr("Document Revision");
    trExecute = tr("Execute");
    trTransID = tr("Transaction ID");
    trTotalAmount = tr("Total Amount");
    trStored = tr("Stored");

    documentModel = new QStringListModel(this);
    ui->listViewDocuments->setModel(documentModel);
    ui->listViewDocuments->setEditTriggers(QAbstractItemView::NoEditTriggers);
    connect(ui->listViewDocuments->selectionModel(),
        SIGNAL(currentChanged(const QModelIndex& , const QModelIndex&)), this,
        SLOT(onlistViewDocumentsChanged(const QModelIndex&, const QModelIndex&)),
        Qt::QueuedConnection);

    LoadFiles();

    ui->pushButtonOpenFile->setEnabled(false);
    ui->pushButtonRevision->setEnabled(false);
}

void DocumentList::setClientModel(ClientModel *model)
{
    this->clientModel = model;
}

void DocumentList::setWalletModel(WalletModel *model)
{
    this->walletModel = model;
}

DocumentList::~DocumentList()
{
    delete documentModel;
    delete ui;
}

QString DocumentList::fileDir() const
{
    QString dstdir = GUIUtil::boostPathToQString(GetDataDir() / "documents/desc");

    QDir dir(dstdir);
    if (!dir.exists())
        dir.mkpath(dstdir);

    dstdir = GUIUtil::boostPathToQString(GetDataDir() / "documents");

    return dstdir;
}

QString DocumentList::getFileName(const QModelIndex &index, const bool fullPath)
{
    QString fn = (fullPath ? fileDir() + "/" : "") + documentModel->stringList().at(index.row());
    return QDir::toNativeSeparators(fn);
}

QString DocumentList::selectedFileName(const bool fullPath)
{
    QModelIndex index = ui->listViewDocuments->currentIndex();
    return index.isValid() ? getFileName(index, fullPath) : "";
}

void DocumentList::selectDocument(const QString docFileName)
{
    QString name = GUIUtil::extractFileName(docFileName);
    QStringList list = documentModel->stringList();

    // find index (indexOf and match does not work directly after LoadFiles)
    int idx = -1;
    int i;
    for (i=0; i<list.count(); i++) {
        if (list.at(i) == name) {
            idx = i;
            break;
        }
    }
    if (idx > -1) {
        QModelIndex midx = documentModel->index(idx);
        if (midx.isValid())
          ui->listViewDocuments->setCurrentIndex(midx);
    }
}

bool DocumentList::TransactionConfirmDlg(const QString docName, const double txFee)
{
    CAmount txFeeSat = txFee * COIN;

    QString questionString = tr("Are you sure you want to send?");
    questionString.append("<br><br>");
    questionString.append(tr("Store document information"));
    questionString.append("<br><b>");
    questionString.append(docName);
    questionString.append("</b><hr><span style='color:#aa0000;'>");
    questionString.append(BitcoinUnits::formatHtmlWithUnit(walletModel->getOptionsModel()->getDisplayUnit(), txFeeSat));
    questionString.append("</span> ");
    questionString.append(" " + tr("is paid as transaction fee."));

    // add total amount in all subdivision units
    questionString.append("<hr />");
    QStringList alternativeUnits;
    Q_FOREACH(BitcoinUnits::Unit u, BitcoinUnits::availableUnits()) {
        if(u != walletModel->getOptionsModel()->getDisplayUnit())
            alternativeUnits.append(BitcoinUnits::formatHtmlWithUnit(u, txFeeSat));
    }
    questionString.append(tr("Total Amount = <b>%1</b><br />= %2")
        .arg(BitcoinUnits::formatHtmlWithUnit(walletModel->getOptionsModel()->getDisplayUnit(), txFeeSat))
        .arg(alternativeUnits.join("<br />= ")));

    QMessageBox::StandardButton retval = QMessageBox::question(this, tr("Send Document"), 
        questionString,
        QMessageBox::Yes | QMessageBox::Cancel,
        QMessageBox::Yes);
    return (retval == QMessageBox::Yes);
}

QString DocumentList::addFile(const QString srcName)
{
    // copy file to wallets document directory
    QString name = GUIUtil::extractFileName(srcName);
    QString docFileName = fileDir() + "/" +  name;

    bool ok;
    while (QFile::exists(docFileName)) {
        name = QInputDialog::getText(this, tr("File exists"), tr("New file name"),
                                     QLineEdit::Normal, name, &ok);
        if (!ok)
            return "";
        docFileName = fileDir() + "/" +  name;
    }

    if (!QFile::copy(srcName, docFileName))
        return "";

    docFileName = QDir::toNativeSeparators(docFileName);

    // create desc file
    QString txid = "";
    Document doc(docFileName);

    // store document information in blockchain
    txid = doc.writeToBlockchain();

    // delete document file if it has not been saved in blockchain
    if (txid.isEmpty()) {
        QFile::remove(doc.filename);
        QFile::remove(doc.descfilename);
        return "";
    }

    // add blockchain info to description file
    QSettings descFile(doc.descfilename, QSettings::IniFormat);
    descFile.beginGroup("blockchain");
    descFile.setValue("txid", txid);
    descFile.endGroup();
    descFile.sync();

    return docFileName;
}

QString DocumentList::addFiles(const QStringList srcFiles)
{
    if(!walletModel || !walletModel->getOptionsModel())
        return "";

    QString lastAddedFile;

    double dlgSumFee = minDocRevFee * srcFiles.count();
    QString dlgDocName = srcFiles.count() == 1 ? GUIUtil::extractFileName(srcFiles.at(0)) : tr("%1 document files").arg(srcFiles.count());

    /** request unlock if wallet is locked or unlocked for mixing only
       RPC: use "getencryptionstatus", if not "unencrypted" or "unlocked"
            call "walletpassphrase \"passphrase\" timeout"
    */
    WalletModel::EncryptionStatus encStatus = walletModel->getEncryptionStatus();
    if(encStatus == walletModel->Locked || encStatus == walletModel->UnlockedForMixingOnly) {
        WalletModel::UnlockContext ctx(walletModel->requestUnlock());
        if(ctx.isValid() && TransactionConfirmDlg(dlgDocName, dlgSumFee)) {
            for (const auto& srcFile : srcFiles) {
                if (QFile::exists(srcFile))
                    lastAddedFile = addFile(srcFile);
            }
        }
    }
    else if (TransactionConfirmDlg(dlgDocName, dlgSumFee)) {
        for (const auto& srcFile : srcFiles) {
            if (QFile::exists(srcFile))
                lastAddedFile = addFile(srcFile);
        }
    }

    return lastAddedFile;
}

void DocumentList::documentRevision(const QString docFileName)
{
    Document doc(docFileName);
    ui->textBrowserRevision->setHtml(doc.getInformationHtml());
    ui->textBrowserRevision->append(doc.documentRevision());
}

void DocumentList::LoadFiles()
{
    QDir dir(fileDir());
    QStringList fileList = dir.entryList(QStringList() << "*.*",QDir::Files);

    documentModel->setStringList(fileList);
}

/** file drag&drop from BitcoinGUI::dropEvent */
void DocumentList::handleNewFiles(const QStringList newFiles)
{
    QString lastAddedFile = addFiles(newFiles);

    LoadFiles();
    selectDocument(lastAddedFile);
}

void DocumentList::onlistViewDocumentsChanged(const QModelIndex &current, const QModelIndex &previous)
{
    if (!current.isValid()) {
        ui->pushButtonOpenFile->setEnabled(false);
        ui->pushButtonRevision->setEnabled(false);
        return;
    }

    Document doc(getFileName(current, true));
    ui->textBrowserRevision->setHtml(doc.getInformationHtml());
    ui->textBrowserRevision->append(QString("<h2>%1</h2><p><a href=\"docrev\">%2</a></p>")
                                           .arg(trDocRev, trExecute));

    ui->pushButtonOpenFile->setEnabled(true);
    ui->pushButtonRevision->setEnabled(true);
}

void DocumentList::on_textBrowserRevision_anchorClicked(const QUrl &arg1)
{
    QString cmd = arg1.toString();

    if (cmd == "open")
        Q_EMIT on_pushButtonOpenFile_clicked();
    else if (cmd == "docrev")
        Q_EMIT on_pushButtonRevision_clicked();
    else
        QDesktopServices::openUrl(arg1);
}

void DocumentList::on_listViewDocuments_doubleClicked(const QModelIndex &index)
{
    GUIUtil::openDocumentFile(getFileName(index, true));
}

void DocumentList::on_pushButtonAddFile_clicked()
{
    QStringList srcFileNames = QFileDialog::getOpenFileNames(this, 
        tr("Select file(s) to append"), 
        GUIUtil::getOSDocumentsDir(), tr("All Files (*.*)"));
    if (srcFileNames.isEmpty()) return;

    QString lastAddedFile = addFiles(srcFileNames);

    LoadFiles();
    selectDocument(lastAddedFile);
}

void DocumentList::on_pushButtonOpenFile_clicked()
{
    GUIUtil::openDocumentFile(selectedFileName(true));
}

void DocumentList::on_pushButtonRevision_clicked()
{
    documentRevision(selectedFileName(true));
}
