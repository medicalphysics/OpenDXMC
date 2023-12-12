/*This file is part of OpenDXMC.

OpenDXMC is free software : you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

OpenDXMC is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with OpenDXMC. If not, see < https://www.gnu.org/licenses/>.

Copyright 2023 Erlend Andersen
*/

#include <bowtiefilterreader.hpp>

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>

#include <utility>
#include <vector>

BowtieFilterReader::BowtieFilterReader(QObject* parent)
    : QObject(parent)
{
}

QJsonDocument readJsonFile(const QString& path)
{

    QFile fil(path);
    if (!fil.open(QIODevice::ReadOnly)) {
        return QJsonDocument {};
    }

    auto txt = fil.readAll();
    fil.close();

    auto doc = QJsonDocument::fromJson(txt);

    return doc;
}

QMap<QString, BowtieFilter> getFilters(const QJsonDocument& doc)
{
    if (doc.isEmpty())
        return QMap<QString, BowtieFilter> {};
    if (!doc.isObject())
        return QMap<QString, BowtieFilter> {};

    auto json = doc.object();
    if (!json.contains("filters"))
        return QMap<QString, BowtieFilter> {};
    if (!json["filters"].isArray())
        return QMap<QString, BowtieFilter> {};

    QMap<QString, BowtieFilter> res;

    auto filterarray = json["filters"].toArray();
    for (const auto& filter_v : filterarray) {
        if (filter_v.isObject()) {
            const auto& filter = filter_v.toObject();
            QString name;
            if (filter.contains("name")) {
                name = filter["name"].toString();
            }
            std::vector<std::pair<double, double>> data;
            data.reserve(10);
            if (filter.contains("filterdata")) {
                const auto& filterdata = filter["filterdata"].toArray();
                for (const auto& fdata : filterdata) {
                    if (fdata.isObject()) {
                        const auto& d = fdata.toObject();
                        if (d.contains("angle") && d.contains("weight")) {
                            if (d["angle"].isDouble() && d["weight"].isDouble()) {
                                auto angle = d["angle"].toDouble();
                                auto weight = d["weight"].toDouble();
                                data.push_back(std::make_pair(angle, weight));
                            }
                        }
                    }
                }
            }
            if (data.size() > 0 && name.size() > 0) {
                res[name] = BowtieFilter(data);
            }
        }
    }
    return res;
}

QMap<QString, BowtieFilter> BowtieFilterReader::read(const QStringList& filepaths)
{
    QMap<QString, BowtieFilter> res;
    for (const auto& path : filepaths) {
        auto fmap = read(path);
        const auto keys = fmap.keys();
        for (const auto& key : keys) {
            res[key] = fmap[key];
        }
    }
    return res;
}

QMap<QString, BowtieFilter> BowtieFilterReader::read(const QString& filepath)
{
    auto doc = readJsonFile(filepath);
    auto filters = getFilters(doc);
    return filters;
}
