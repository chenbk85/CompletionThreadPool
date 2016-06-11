#ifndef PUBLISHER_H
#define PUBLISHER_H

#include <QObject>
#include <QPixmap>
#include <QImage>
#include <QLabel>
#include <vector>

class Publisher : public QObject
{
    Q_OBJECT

    std::vector<QLabel*> imageViews_;

public:
    Publisher(std::vector<QLabel*> imageViews) {
        imageViews_ = imageViews;
    }

public slots:
    void publishSlot(QImage qImage) {
        QPixmap qPixmap;

        if (qPixmap.convertFromImage(qImage)) {

            int height = qPixmap.size().height();
            std::string qLabelObjectName("label" + std::to_string(height));

            auto it = std::find_if(imageViews_.begin(), imageViews_.end(),
                                   [&qLabelObjectName] (const QLabel* qLabel) {
                return qLabel->objectName() == qLabelObjectName.c_str();
            });

            if (it == imageViews_.end()) {
                it = imageViews_.begin();
            }

            if (it != imageViews_.end()) {
                QLabel* qLabel = *it;
                qLabel->setPixmap(qPixmap);
            }
        }
    }
};

#endif // PUBLISHER_H
