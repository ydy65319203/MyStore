#ifndef CMYWIDGET_H
#define CMYWIDGET_H

#include <QWidget>
#include <QMouseEvent>

class CMyWidget : public QWidget
{
    Q_OBJECT
public:
    explicit CMyWidget(QWidget *parent = nullptr);

protected:
    void enterEvent(QEvent *) override;  //进入QWidget瞬间事件
    void leaveEvent(QEvent *) override;  //离开QWidget瞬间事件
    void mouseMoveEvent(QMouseEvent *event) override;

signals:

};

#endif // CMYWIDGET_H
