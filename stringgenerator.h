﻿#ifndef STRINGGENERATOR_H
#define STRINGGENERATOR_H
#include <QtCore>

/**
 * @brief The StringGenerator class
 * 生成界面显示字符串
 */
class StringGenerator
{
public:
    StringGenerator();
    static QString getString(const QMap<QString,QString> &messageMap);
private:
    static QString TE(QString color, QString font_family, QString txt);

};

#endif // STRINGGENERATOR_H
