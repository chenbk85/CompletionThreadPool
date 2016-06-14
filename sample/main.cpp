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
#include <atomic>
#include "publisher.h"
#include "../CompletionThreadPool.h"

QImage downloadSampleImage(const struct addrinfo* serverinfo,
                           const std::string& host,
                           const std::string& path) {
    const int MAXDATASIZE = 256;
    char buf[MAXDATASIZE];
    memset(buf, 0, MAXDATASIZE);
    int sockfd, numbytes;

    QImage qimage;

    const addrinfo* p;
    for(p = serverinfo; p != NULL; p = p->ai_next) {
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

    std::string response;
    while ((numbytes = recv(sockfd, buf, MAXDATASIZE, 0)) > 0) {
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

 void workerThreadMain(Publisher &publisher, std::atomic<bool>& stopFlag) {
        addrinfo hints, *servinfo;
        memset(&hints, 0, sizeof hints);
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;

        std::string host("lorempixel.com"), protocol("http");
        int res = getaddrinfo(host.c_str(), protocol.c_str(), &hints, &servinfo);
        if (res == 0) {
            CompletionThreadPool<QImage> completionThreadPool;

            std::vector<std::tuple<addrinfo*, std::string, std::string>> imageUrls = {
                std::make_tuple(servinfo, host.c_str(), "/1000/1000/"),
                std::make_tuple(servinfo, host.c_str(), "/50/50/"),
                std::make_tuple(servinfo, host.c_str(), "/150/150/")
            };

            int submittedCount = 0;
            auto it = imageUrls.begin();
            while (it != imageUrls.end()) {
                submittedCount += completionThreadPool.Submit(downloadSampleImage,
                                                              std::get<0>(*it),
                                                              std::get<1>(*it),
                                                              std::get<2>(*it));
                ++it;
            }

            while (submittedCount--) {
                if (stopFlag) break;
                QImage qimage = completionThreadPool.Take().get();
                if (qimage.valid(0,0)) {
                    QMetaObject::invokeMethod(&publisher, "publishSlot", Qt::QueuedConnection,
                                              Q_ARG(QImage, qimage));
                }
            }
        } else {
            std::cout << "getaddrinfo: " << gai_strerror(res) << std::endl;
        }

        freeaddrinfo(servinfo);
 }

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    std::vector<QLabel*> imageViews = {
        w.findChild<QLabel*>("label50"),
        w.findChild<QLabel*>("label150"),
        w.findChild<QLabel*>("label1000")
    };

    Publisher publisher(imageViews);
    std::atomic<bool> stopFlag(false);
    std::thread workerThread(workerThreadMain, std::ref(publisher), std::ref(stopFlag));

    int res = a.exec();
    stopFlag = true;
    workerThread.join();
    return res;
}
