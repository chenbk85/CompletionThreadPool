#include "mainwindow.h"
#include <QApplication>
#include <cstdlib>
#include <iostream>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <iostream>
#include <QImageReader>
#include <QDebug>
#include <QFile>
#include <QLabel>
#include <vector>
#include <string>
#include "publisher.h"
#include "../CompletionThreadPool.h"

QImage downloadSampleImage(const std::string& protocol, const std::string& host,
                           const std::string& path) {
    const int MAXDATASIZE = 256;
    char buf[MAXDATASIZE];
    memset(buf, 0, 100);
    struct addrinfo hints, *servinfo, *p;
    int sockfd, numbytes;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    std::cout << "Getting " << path.c_str() << std::endl;

    QImage qimage;

    if (getaddrinfo(host.c_str(), protocol.c_str(), &hints, &servinfo) != 0) {
        return qimage;
    }

    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                             p->ai_protocol)) == -1) {
            continue;
        }

        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            continue;
        }

        break;
    }

    if (p == NULL) {
        return qimage;
    }

    std::string request("GET ");
    request.append(path);
    request.append(" HTTP/1.0\r\nHost:");
    request.append(host);
    request.append("\r\n\r\n");

    if (send(sockfd, request.c_str(), request.size(), 0) < 0) {
        return qimage;
    }

    freeaddrinfo(servinfo);

    std::string response;
    while ((numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0)) > 0) {
        if (numbytes == -1)
            return qimage;

        response.append(buf, buf+numbytes);
    }

    std::size_t index = response.find(std::string("\r\n\r\n"));
    if (index != std::string::npos) {
        std::string imageData = response.substr(index+4);
        qimage = QImage::fromData((const uchar*)&imageData.front(), imageData.size());

    }

    close(sockfd);
    return qimage;
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    std::vector<std::tuple<std::string, std::string, std::string>> imageUrls = {
        std::make_tuple("http", "lorempixel.com", "/1000/1000/"),
        std::make_tuple("http", "lorempixel.com", "/50/50/"),
        std::make_tuple("http", "lorempixel.com", "/150/150/")
    };

    std::vector<QLabel*> imageViews = {
        w.findChild<QLabel*>("label150"),
        w.findChild<QLabel*>("label50"),
        w.findChild<QLabel*>("label1000")
    };

    Publisher publisher(imageViews);

    std::atomic_bool stopFlag(false);
    std::thread workerThread([&stopFlag, &publisher, &imageUrls]() {
        auto it = imageUrls.begin();
        CompletionThreadPool<QImage> completionThreadPool;

        int submittedCount = 0;
        while (it != imageUrls.end() && !stopFlag) {
            submittedCount += completionThreadPool.Submit(downloadSampleImage,
                                                          std::get<0>(*it),
                                                          std::get<1>(*it),
                                                          std::get<2>(*it));
            ++it;
        }

        while (submittedCount--) {
            QImage qimage = completionThreadPool.Take().get();
            if (qimage.valid(0,0)) {
                QMetaObject::invokeMethod(&publisher, "publishSlot", Qt::QueuedConnection,
                                          Q_ARG(QImage, qimage));
            }
        }
    });

    int res = a.exec();
    stopFlag = true;
    workerThread.join();
    return res;
}
