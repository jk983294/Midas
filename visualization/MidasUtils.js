var mathUtils = (function() {

    function getAlpha(interval) {
        if (interval > 3) {
            return 2.0 / (interval + 1);
        }
        return 0.7;
    }

    function calculateSma(data, interval) {
        var result = [];
        var sum = 0;
        for (var i = 0; i < data.length; i++) {
            sum += data[i];
            if (i < interval) {
                result.push((sum / (i + 1)).toFixed(4));
            } else {
                sum -= data[i - interval];
                result.push((sum / interval).toFixed(4));
            }
        }
        return result;
    }

    function calculateEma(data, interval) {
        var alpha = getAlpha(interval);
        var result = [];
        result.push(data[0]);
        for (var i = 1; i < data.length; i++) {
            result.push((alpha * data[i] + (1 - alpha) * result[i - 1]).toFixed(4));
        }
        return result;
    }

    function calculateMacd(price) {
        if (!price) return {
            macd: [],
            dif: [],
            dea: []
        };

        var pMaFast = calculateEma(price, 12);
        var pMaSlow = calculateEma(price, 26);
        var dif = [];

        for (var j = 0; j < price.length; j++) {
            dif[j] = (pMaFast[j] - pMaSlow[j]).toFixed(4);
        }

        var dea = calculateEma(dif, 9);
        var macd = [];
        for (var k = 0; k < price.length; k++) {
            macd[k] = ((dif[k] - dea[k]) * 2.0).toFixed(4);
        }
        return {
            macd: macd,
            dif: dif,
            dea: dea
        };
    }

    /**
     * Statistics Functions
     */
    function standardDeviation(values) {
        var avg = average(values);

        var squareDiffs = values.map(function(value) {
            var diff = value - avg;
            return diff * diff;
        });

        var avgSquareDiff = average(squareDiffs);

        return Math.sqrt(avgSquareDiff);
    }

    function average(data) {
        var sum = data.reduce(function(sum, value) {
            return sum + value;
        }, 0);

        return sum / data.length;
    }

    function changePct(data) {
        var result = [];
        result.push(0.0);
        for (var i = 1; i < data.length; i++) {
            result.push(((data[i] / data[i - 1]) - 1.0).toFixed(4));
        }
        return result;
    }


    return {
        calculateSma: calculateSma,
        calculateEma: calculateEma,
        calculateMacd　: 　calculateMacd,
        standardDeviation: standardDeviation,
        average: average,
        changePct: changePct
    };
})();

var midasUtils = (function() {

    function row2column(rows, rowMeta) {
        var result = {};

        if (rows && rows.length) {
            for (var prop in rowMeta) {
                if (rowMeta.hasOwnProperty(prop)) {
                    result[prop] = [];
                }
            }
            for (var i = 0, len = rows.length; i < len; ++i) {
                for (var key in rowMeta) {
                    if (rowMeta.hasOwnProperty(key)) {
                        result[key].push(rows[i][rowMeta[key]]);
                    }
                }
            }
        }
        return result;
    }

    function subArrayOfObject(object, index1, index2) {
        var result = {};
        for (var key in object) {
            if (object.hasOwnProperty(key)) {
                if (object[key] instanceof Array) {
                    result[key] = object[key].slice(index1, index2);
                } else {
                    result[key] = object[key];
                }
            }
        }
        return result;
    }

    function rangeArray(len) {
        var data = [];
        for (var i = 0; i < len; ++i) data.push(i);
        return data;
    }

    function arrayConcat(array, str, isbefore) {
        var data = [];
        for (var i = 0, len = array.length; i < len; ++i) {
            data.push(isbefore ? str + ' ' + array[i] : array[i] + ' ' + str);
        }
        return data;
    }

    /**
     * data is an object array, extract those object's property to an array
     */
    function extractProperty(array, property) {
        var properties = [];
        for (var i = 0, len = array.length; i < len; i++) properties.push(array[i][property]);
        return properties;
    }

    /**
     * data is [{x : x1, y: y1, filter : filter}],
     * get {label: yName, data : [[x1, y1]]} and filter between start and end if exists filter condition
     */
    function extractTimeSeries(array, xName, yName, filterName, start, end) {
        var results = [];
        var minDay, maxDay;
        if (filterName) {
            minDay = Math.min(start, end);
            maxDay = Math.max(start, end);
        }
        for (var i = 0, len = array.length; i < len; i++) {
            if (array[i][xName] && array[i][yName]) {
                if (!filterName || (array[i][filterName] >= minDay && array[i][filterName] <= maxDay)) {
                    results.push([array[i][xName], array[i][yName]]);
                }
            }
        }
        return {
            label: yName,
            data: results
        };
    }


    function isInArray(array, toSearch) {
        for (var i = 0, len = array.length; i < len; i++) {
            if (array[i] === toSearch) return true;
        }
        return false;
    }

    /**
     * remove something in array
     */
    function array2remove(array, toRemove) {
        var index = array.indexOf(toRemove);
        if (index > -1) {
            array.splice(index, 1);
        }
        return array;
    }

    /**
     * array and object converter
     */
    function array2object(array) {
        var object = {};
        for (var i = 0, len = array.length; i < len; ++i) {
            object[array[i]] = array[i];
        }
        return object;
    }

    function object2array(object) {
        var array = [];
        for (var key in object) {
            if (object.hasOwnProperty(key) && key !== 'date') {
                array.push(key);
            }
        }
        return array;
    }

    function object2PropArray(object) {
        var array = [];
        for (var prop in object) {
            if (object.hasOwnProperty(prop) && !(prop.startsWith("$") || prop === 'toJSON')) {
                array.push({
                    name: prop,
                    value: object[prop]
                });
            }
        }
        return array;
    }

    /**
     * merge two array into one array
     */
    function merge2Array(p1, p2) {
        var properties = [];
        for (var i = 0, len = p1.length; i < len; i++) properties.push([p1[i], p2[i]]);
        return properties;
    }

    function mergeForD3Point(x, y) {
        var data = [];
        for (var i = 0, len = x.length; i < len; i++) data.push({
            x: x[i],
            y: y[i]
        });
        return data;
    }

    function isNull() {
        for (var i = 0; i < arguments.length; i++) {
            var data = arguments[i];
            if (data === null || data === undefined || (typeof data === 'string' && data === '') ||
                (data instanceof Array && data.length === 0)) {
                return true;
            }
        }
        return false;
    }

    function errorHandler(msg) {
        console.error('Failure : ', msg);
    }

    /**
     * date and int converter
     */
    function toTimes(dates) {
        var times = [];
        for (var i = 0, len = dates.length; i < len; ++i) {
            times[i] = toTime(dates[i]);
        }
        return times;
    }

    function toTime(date) {
        var year = Math.floor(date / 10000);
        var month = Math.floor(date / 100) % 100;
        var day = date % 100;
        return new Date(year, month - 1, day).getTime();
    }

    function date2int(date) {
        if (!date) return null;
        if (date instanceof Date) {
            return date.getDate() + (date.getMonth() + 1) * 100 + date.getFullYear() * 10000;
        } else { // seconds from 1970.1.1
            date = new Date(date);
            return date.getDate() + (date.getMonth() + 1) * 100 + date.getFullYear() * 10000;
        }
    }

    function toCobs(dates) {
        var times = [];
        for (var i = 0, len = dates.length; i < len; ++i) {
            times[i] = date2int(dates[i]);
        }
        return times;
    }

    /**
     * if not found, return the up bound, but not exceed the array length
     */
    function binaryIndexOf(array, searchElement) {
        var len = array.length - 1;
        var minIndex = 0;
        var maxIndex = len;
        var currentIndex = 0;
        while (minIndex <= maxIndex) {
            currentIndex = Math.floor((minIndex + maxIndex) / 2);
            if (array[currentIndex] < searchElement) {
                minIndex = currentIndex + 1;
            } else if (array[currentIndex] > searchElement) {
                maxIndex = currentIndex - 1;
            } else {
                return currentIndex;
            }
        }
        return (minIndex > len) ? (len === 0 ? 0 : -len) : (minIndex === 0 ? 0 : -minIndex);
    }

    function deviateLevel(value, benchmark) {
        return Math.abs((value - benchmark) / benchmark);
    }

    return {
        array2object: array2object,
        object2array: object2array,
        object2PropArray: object2PropArray,
        extractProperty: extractProperty,
        extractTimeSeries: extractTimeSeries,
        merge2Array: merge2Array,
        mergeForD3Point: mergeForD3Point,
        binaryIndexOf: binaryIndexOf,
        array2remove: array2remove,
        isInArray: isInArray,
        rangeArray: rangeArray,
        arrayConcat: arrayConcat,
        row2column: row2column,
        subArrayOfObject: subArrayOfObject,

        isNull: isNull,
        errorHandler: errorHandler,

        toTimes: toTimes,
        toTime: toTime,
        date2int: date2int,
        toCobs: toCobs,

        deviateLevel: deviateLevel
    };
})();
