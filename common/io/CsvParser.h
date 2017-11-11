#ifndef MIDAS_CSV_PARSER_H
#define MIDAS_CSV_PARSER_H

#include <list>
#include <map>
#include <string>
#include <vector>
#include "utils/convert/StringHelper.h"

using namespace std;

namespace midas {

class CsvParser {
public:
    bool hasHeader{true};
    int dataCount{0};  // number of column
    char delimiter{','};

    map<string, vector<string>> columns;
    vector<vector<string>> rows;
    vector<string> header;
    map<string, int> column2index;
    map<string, string> metadata;  // key value of metadata

public:
    CsvParser() {}

    void parse(string filePath) {
        clear();

        ifstream ifs(filePath, ifstream::in);

        if (ifs.is_open()) {
            string line;
            while (getline(ifs, line)) {
                boost::trim(line);
                if (line.size() > 0) {
                    vector<string> lets = string_split(line, delimiter);

                    if (hasHeader && header.size() == 0) {
                        header = lets;
                    } else {
                        rows.push_back(lets);
                    }
                }
            }

            ifs.close();
        }

        if (rows.size() == 0) return;

        parse_header();

        vector<vector<string>> columnArray;
        columnArray.resize(dataCount);

        int cnt = 0;
        for (vector<string>& row : rows) {
            if (row.size() == dataCount) {
                for (int i = 0; i < dataCount; ++i) {
                    columnArray[i].push_back(row[i]);
                }
            } else if (row.size() == dataCount - 1) {
                for (int i = 0; i < dataCount - 1; ++i) {
                    columnArray[i].push_back(row[i]);
                }
                // last value can be null, like a,b,c,
                columnArray[dataCount - 1].push_back("null");
            } else {
                THROW_MIDAS_EXCEPTION(filePath << " : data count not correct in row " << cnt);
            }
            cnt++;
        }

        int missingCount = 0;
        for (int i = 0; i < dataCount; ++i) {
            string columnName = header.get(i);
            if (StringUtils.isEmpty(columnName)) {
                columnName = "default" + missingCount;
                missingCount++;
            }
            columns[columnName] = columnArray[i];
            column2index.put(columnName, i);
        }
    }

    void parse_header() {
        if (hasHeader) {
            dataCount = header.size();
        } else {
            dataCount = rows[0].size();
            for (size_t i = 0; i < dataCount; ++i) {
                header.push_back("header" + i);
            }
        }
    }

    void clear() {
        columns.clear();
        rows.clear();
        header.clear();
        column2index.clear();
        metadata.clear();
        dataCount = 0;
    }

    string getRowAt(vector<string> row, int columnIndex) { return row.get(columnIndex); }

    string getRowAt(vector<string> row, string column) { return row.get(column2index.get(column)); }
};
}

#endif
