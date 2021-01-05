#include "mywidget.h"
#include "mylog.h"

CMyWidget::CMyWidget(QWidget *parent) : QWidget(parent)
{
    this->setMouseTracking(true);
}


void CMyWidget::enterEvent(QEvent *event)
{
    LOG(Info, "CMyWidget::enterEvent( event->type() = %d )... \n", event->type());

    this->frameGeometry();
    this->geometry();
    this->rect();
}

void CMyWidget::leaveEvent(QEvent *event)
{
    LOG(Info, "CMyWidget::leaveEvent( event->type() = %d )... \n", event->type());
}

void CMyWidget::mouseMoveEvent(QMouseEvent *event)
{
    LOG(Info, "CMyWidget::mouseMoveEvent(x=%d, globalX=%d)... \n", event->x(), event->globalX());
}
