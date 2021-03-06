#include "GUI.hpp"
#include <QPushButton>
#include <QLabel>
#include <QImage>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QListWidget>
#include <QFileDialog>
#include <igtl/igtlImageMessage.h>
#include <igtl/igtlTransformMessage.h>
#include <igtl/igtlOSUtil.h>
#include <FAST/Streamers/ImageFileStreamer.hpp>
#include <FAST/Data/Image.hpp>
#include <include/QtCore/QMetaType>
#include <FAST/Tests/DummyObjects.hpp>

namespace fast {

GUI::GUI() {
    mRunning = false;
    mStop = false;
    mPort = 18944;
    mThread = nullptr;
    mFPS = 10;
    setTitle("FAST - OpenIGTLink Server");

    QVBoxLayout* layout = new QVBoxLayout(mWidget);

    // Setup GUI
    // Logo
    QImage* image = new QImage;
    image->load((Config::getDocumentationPath() + "images/FAST_logo_square.png").c_str());
    QLabel* logo = new QLabel;
    logo->setPixmap(QPixmap::fromImage(image->scaled(mWidth, ((float)mWidth/image->width())*image->height(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation)));
    logo->adjustSize();
    layout->addWidget(logo);

    // Title label
    QLabel* title = new QLabel;
    title->setText("<div style=\"text-align: center; font-weight: bold; font-size: 24px;\">OpenIGTLink<br>Server</div>");
    layout->addWidget(title);

    // Quit button
    QPushButton* quitButton = new QPushButton;
    quitButton->setText("Quit (q)");
    quitButton->setStyleSheet("QPushButton { font-size: 24px; background-color: red; color: white; }");
    QObject::connect(quitButton, &QPushButton::clicked, std::bind(&Window::stop, this));
    layout->addWidget(quitButton);


    mStartStopButton = new QPushButton;
    mStartStopButton->setText("Start streaming");
    mStartStopButton->setStyleSheet("QPushButton { font-size: 24px; color: white; background-color: green; }");
    layout->addWidget(mStartStopButton);
    QObject::connect(mStartStopButton, &QPushButton::clicked, std::bind(&GUI::toggleServer, this));

    mStatus = new QLabel;
    mStatus->setText("Current status: Server not running");
    mStatus->setStyleSheet("QLabel { font-size: 24px; }");
    layout->addWidget(mStatus);

    QLabel* listLabel = new QLabel;
    listLabel->setText("Recordings");
    listLabel->setStyleSheet("QLabel { font-size: 24px; }");
    layout->addWidget(listLabel);

    mList = new QListWidget;
    layout->addWidget(mList);

    QPushButton* addButton = new QPushButton;
    addButton->setText("Add recordings");
    QObject::connect(addButton, &QPushButton::clicked, std::bind(&GUI::addRecording, this));
    layout->addWidget(addButton);

    QPushButton* deleteButton = new QPushButton;
    deleteButton->setText("Delete selected recordings");
    QObject::connect(deleteButton, &QPushButton::clicked, std::bind(&GUI::removeRecordings, this));
    layout->addWidget(deleteButton);

    mWidget->setLayout(layout);
    mStartStopButton->setFocus();
}

void GUI::addRecording() {
    QFileDialog fileDialog(mWidget);
    fileDialog.setNameFilter("Image recordings (*_0.mhd *_0.png *_0.jpg *_0.bmp)");
    QStringList filenames;
    if(fileDialog.exec()) {
        filenames = fileDialog.selectedFiles();
        for(QString qfilename : filenames) {
            std::string filename = qfilename.toStdString();
            filename = replace(filename, "_0.", "_#.");
            mList->addItem(filename.c_str());
        }
    }
}

void GUI::removeRecordings() {
    qDeleteAll(mList->selectedItems());
}

void GUI::toggleServer() {
    if(!mRunning) {
        // Add recordings
        mFilenameFormats.clear();
        for(int i = 0; i < mList->count(); ++i) {
            QString filename = mList->item(i)->text();
            mFilenameFormats.push_back(filename.toStdString());
        }
        if(mFilenameFormats.size() == 0) {
            QMessageBox* message = new QMessageBox(mWidget);
            message->setWindowTitle("Error");
            message->setText("No recordings selected");
            message->show();
            return;
        }
        // Start server
        mServerSocket = igtl::ServerSocket::New();
        int result = mServerSocket->CreateServer(mPort);
        if(result < 0) {
            QMessageBox* message = new QMessageBox(mWidget);
            message->setWindowTitle("Error");
            message->setText("Unable to create server socket.");
            message->show();
            return;
        }
        mStop = false;
        mThread = new std::thread(std::bind(&GUI::streamData, this));
        mStartStopButton->setText("Stop streaming");
        mStartStopButton->setStyleSheet("QPushButton { font-size: 24px; color: white; background-color: red; }");
    } else {
        // Stop server
        mStop = true;
        // Wait for streamData thread to stop, then delete it
        mThread->join();
        delete mThread;
        mThread = nullptr;

        // Update button
        mStartStopButton->setText("Start streaming");
        mStartStopButton->setStyleSheet("QPushButton { font-size: 24px; color: white; background-color: green; }");
    }
    mRunning = !mRunning;
}

void GUI::setFilenameFormats(std::vector<std::string> formats) {
    for(auto format : formats)
        mList->addItem(format.c_str());
}

GUI::~GUI() {
    if(mThread != nullptr) {
        mStop = true;
        mThread->join();
        delete mThread;
    }
}


inline igtl::ImageMessage::Pointer createIGTLImageMessage(Image::pointer image) {
    // size parameters
    int   size[3]     = {(int)image->getWidth(), (int)image->getHeight(), (int)image->getDepth()};       // image dimension
    float spacing[3]  = {image->getSpacing().x(), image->getSpacing().y(), image->getSpacing().z()};     // spacing (mm/pixel)
    int   svoffset[3] = {0, 0, 0};           // sub-volume offset
    int   scalarType;
    size_t totalSize = image->getWidth()*image->getHeight()*image->getDepth()*image->getNrOfComponents();
    switch(image->getDataType()) {
        case TYPE_UINT8:
            scalarType = igtl::ImageMessage::TYPE_UINT8;
            totalSize *= sizeof(unsigned char);
            break;
        case TYPE_INT8:
            scalarType = igtl::ImageMessage::TYPE_INT8;
            totalSize *= sizeof(char);
            break;
        case TYPE_UINT16:
            scalarType = igtl::ImageMessage::TYPE_UINT16;
            totalSize *= sizeof(unsigned short);
            break;
        case TYPE_INT16:
            scalarType = igtl::ImageMessage::TYPE_INT16;
            totalSize *= sizeof(short);
            break;
        case TYPE_FLOAT:
            scalarType = igtl::ImageMessage::TYPE_FLOAT32;
            totalSize *= sizeof(float);
            break;
    }

    //------------------------------------------------------------
    // Create a new IMAGE type message
    igtl::ImageMessage::Pointer imgMsg = igtl::ImageMessage::New();
    imgMsg->SetDimensions(size);
    imgMsg->SetSpacing(spacing);
    imgMsg->SetNumComponents(image->getNrOfComponents());
    imgMsg->SetScalarType(scalarType);
    imgMsg->SetDeviceName("DummyImage");
    imgMsg->SetSubVolume(size, svoffset);
    imgMsg->AllocateScalars();

    ImageAccess::pointer access = image->getImageAccess(ACCESS_READ);
    memcpy(imgMsg->GetScalarPointer(), access->get(), totalSize);

    return imgMsg;
}


void GUI::streamData() {
	// Hack to make this work on windows (get issues with switching GL context to thread): 
	// 1. Create a new GL context in this thread	
	QGLWidget* widget = new QGLWidget;
	QGLContext* mainGLContext = new QGLContext(QGLFormat::defaultFormat(), widget); // by including widget here the context becomes valid
	mainGLContext->create();
	if (!mainGLContext->isValid()) {
		throw Exception("Qt GL context is invalid!");
	}
	// 2. Substitute the GL context in the window with this one
	mMainGLContext = mainGLContext;
	mainGLContext->makeCurrent();

    std::chrono::duration<int, std::milli> interaval(1000 / mFPS);
    reportInfo() << "Listening for new connections on port " << mPort << reportEnd();
    try {
        while(true) {
            // Waiting for Connection
            igtl::ClientSocket::Pointer socket = mServerSocket->WaitForConnection(1000);
            mStatus->setText("Current status: No connections");
            if(socket.IsNotNull()) { // if client connected
                std::string clientAddress;
                int clientPort;
                socket->GetSocketAddressAndPort(clientAddress, clientPort);
                reportInfo() << "Connection established with client " << clientAddress << reportEnd();
                mStatus->setText(("Current status: 1 connection - " + clientAddress + ":" + std::to_string(clientPort)).c_str());
                DummyProcessObject::pointer dummy = DummyProcessObject::New();
                ImageFileStreamer::pointer dataStreamer = ImageFileStreamer::New();
                dataStreamer->enableLooping();
                dataStreamer->setFilenameFormats(mFilenameFormats);
                dataStreamer->setStreamingMode(STREAMING_MODE_PROCESS_ALL_FRAMES);
                dataStreamer->update();
                DynamicData::pointer dataStream = dataStreamer->getOutputData<Image>();
                long unsigned int framesSent = 0;
                while(true) {
                    if(mStop) {
                        break;
                    }

                    // Stream images
                    Image::pointer image = dataStream->getNextFrame(dummy);

                    // Create a new IMAGE type message
                    igtl::ImageMessage::Pointer imgMsg = createIGTLImageMessage(image);

                    imgMsg->Pack();
                    reportInfo() << "Sending image frame " << framesSent << reportEnd();
                    int result = socket->Send(imgMsg->GetPackPointer(), imgMsg->GetPackSize());
                    if(result == 0) {
                        reportInfo() << "Connection lost" << reportEnd();
                        break;
                    }

                    std::this_thread::sleep_for(interaval);
                    framesSent++;
                }
            }

            if(mStop) {
                break;
            }
        }
    } catch(std::exception &e) {
        reportInfo() << "Exception caugt while streaming data" << reportEnd();
        mServerSocket->CloseSocket();
        mStatus->setText("Current status: Server not running");
        throw e;
    }

    reportInfo() << "Closing server socket" << reportEnd();
    mStatus->setText("Current status: Server not running");
    mServerSocket->CloseSocket();
}

}