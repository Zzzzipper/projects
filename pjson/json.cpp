#include "json.h"
#include <stdlib.h>
#include <string>
#include <algorithm>
#include <cstdlib>
#include <cstdio>
#include <climits>
#include <string.h>
#include <functional>
#include <cctype>
#include <stack>
#include <cerrno>
#include <inttypes.h>

#ifdef _MSC_VER
#define snprintf sprintf_s
#endif

using namespace json;

namespace json
{

cConfig theConfig;

enum StackDepthType
{
    InObject,
    InArray
};
}

/**
 * @brief Вспомогательные функции
 */
static std::string Trim(const std::string& str)
{
    std::string s = str;
    auto strBegin = 0L;
    auto strEnd = s.size();
    while(strBegin < strEnd) {
        if(std::isspace(static_cast<unsigned char>(s[strBegin]))) {
            ++strBegin;
        } else {
            break;
        }
    }
    while(strEnd > strBegin) {
        strEnd--;
        if(!std::isspace(static_cast<unsigned char>(s[strEnd]))) {
            break;
        }
    }
    return str.substr(strBegin, strEnd - strBegin + 1);
    /* Код не удачный: isspace не защищен от юникода..
    // remove white space in front
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));

    // remove trailing white space
    s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());

    return s;
    */
}

/**
 * @brief Находит позицию первого символа "символ, которому НЕ предшествует символ \.
 * В JSON \ "действительно и имеет другое значение, чем экранированный" символ
 */
static size_t GetQuotePos(const std::string& str, size_t start_pos = 0)
{
    bool found_slash = false;
    for (size_t i = start_pos; i < str.length(); i++)
    {
        char c = str[i];
        if ((c == '\\') && !found_slash)
        {
            found_slash = true;
            continue;
        }
        else if ((c == '\"') && !found_slash)
            return i;

        found_slash = false;
    }

    return std::string::npos;
}

/**
 * @brief Value::Value - сущность элементарной переменной
 * @param v
 */
Value::Value(const Value& v) : mValueType(v.mValueType)
{
    switch (mValueType)
    {
    case StringVal		:
        mStringVal = v.mStringVal;
        break;
    case IntVal			:
        mIntVal = v.mIntVal;
        mFloatVal = (float)v.mIntVal;
        mDoubleVal = (double)v.mIntVal;
        break;
    case FloatVal		:
        mFloatVal = v.mFloatVal;
        mIntVal = (int)v.mFloatVal;
        mDoubleVal = (double)v.mDoubleVal;
        break;
    case DoubleVal		:
        mDoubleVal = v.mDoubleVal;
        mIntVal = (int)v.mDoubleVal;
        mFloatVal = (float)v.mDoubleVal;
        break;
    case BoolVal		:
        mBoolVal = v.mBoolVal;
        break;
    case ObjectVal		:
        mObjectVal = v.mObjectVal;
        break;
    case ArrayVal		:
        mArrayVal = v.mArrayVal;
        break;
    default				:
        break;
    }
}

Value& Value::operator =(const Value& v)
{
    if (&v == this)
        return *this;

    mValueType = v.mValueType;

    switch (mValueType)
    {
    case StringVal		:
        mStringVal = v.mStringVal;
        break;
    case IntVal			:
        mIntVal = v.mIntVal;
        mFloatVal = (float)v.mIntVal;
        mDoubleVal = (double)v.mIntVal;
        break;
    case FloatVal		:
        mFloatVal = v.mFloatVal;
        mIntVal = (int)v.mFloatVal;
        mDoubleVal = (double)v.mDoubleVal;
        break;
    case DoubleVal		:
        mDoubleVal = v.mDoubleVal;
        mIntVal = (int)v.mDoubleVal;
        mFloatVal = (float)v.mDoubleVal;
        break;
    case BoolVal		:
        mBoolVal = v.mBoolVal;
        break;
    case ObjectVal		:
        mObjectVal = v.mObjectVal;
        break;
    case ArrayVal		:
        mArrayVal = v.mArrayVal;
        break;
    default				:
        break;
    }

    return *this;
}

Value& Value::operator [](size_t idx)
{
    if (mValueType != ArrayVal)
        throw std::runtime_error("json mValueType==ArrayVal required");

    return mArrayVal[idx];
}

const Value& Value::operator [](size_t idx) const
{
    if (mValueType != ArrayVal)
        throw std::runtime_error("json mValueType==ArrayVal required");

    return mArrayVal[idx];
}

Value& Value::operator [](const std::string& key)
{
    if (mValueType != ObjectVal)
        throw std::runtime_error("json mValueType==ObjectVal required");

    return mObjectVal[key];
}

Value& Value::operator [](const char* key)
{
    if (mValueType != ObjectVal)
        throw std::runtime_error("json mValueType==ObjectVal required");

    return mObjectVal[key];
}

const Value& Value::operator [](const char* key) const
{
    if (mValueType != ObjectVal)
        throw std::runtime_error("json mValueType==ObjectVal required");

    return mObjectVal[key];
}

const Value& Value::operator [](const std::string& key) const
{
    if (mValueType != ObjectVal)
        throw std::runtime_error("json mValueType==ObjectVal required");

    return mObjectVal[key];
}

void Value::Clear()
{
    mValueType = NULLVal;
}

size_t Value::size() const
{
    if ((mValueType != ObjectVal) && (mValueType != ArrayVal))
        return 1;

    return mValueType == ObjectVal ? mObjectVal.size() : mArrayVal.size();
}

bool Value::HasKey(const std::string &key) const
{
    if (mValueType != ObjectVal)
        throw std::runtime_error("json mValueType==ObjectVal required");

    return mObjectVal.HasKey(key);
}

int Value::HasKeys(const std::vector<std::string> &keys) const
{
    if (mValueType != ObjectVal)
        throw std::runtime_error("json mValueType==ObjectVal required");

    return mObjectVal.HasKeys(keys);
}

int Value::HasKeys(const char **keys, int key_count) const
{
    if (mValueType != ObjectVal)
        throw std::runtime_error("json mValueType==ObjectVal required");

    return mObjectVal.HasKeys(keys, key_count);
}

int Value::ToInt() const
{
    if (!IsNumeric())
        throw std::runtime_error("json mValueType==IsNumeric() required");

    return mIntVal;
}

float Value::ToFloat() const
{
    if (!IsNumeric())
        throw std::runtime_error("json mValueType==IsNumeric() required");

    return mFloatVal;
}

double Value::ToDouble() const
{
    if (!IsNumeric())
        throw std::runtime_error("json mValueType==IsNumeric() required");

    return mDoubleVal;
}

bool Value::ToBool() const
{
    if (mValueType != BoolVal)
        throw std::runtime_error("json mValueType==BoolVal required");

    return mBoolVal;
}

const std::string& Value::ToString() const
{
    if (mValueType != StringVal)
        throw std::runtime_error("json mValueType==StringVal required");

    return mStringVal;
}

Object Value::ToObject() const
{
    if (mValueType != ObjectVal)
        throw std::runtime_error("json mValueType==ObjectVal required");

    return mObjectVal;
}

Array Value::ToArray() const
{
    if (mValueType != ArrayVal)
        throw std::runtime_error("json mValueType==ArrayVal required");

    return mArrayVal;
}

Array& Value::Array() {
    if (mValueType != ArrayVal)
        throw std::runtime_error("json mValueType==ArrayVal required");

    return mArrayVal;
}

Value::operator int() const
{
    if (!IsNumeric())
        throw std::runtime_error("json mValueType==IsNumeric() required");

    return mIntVal;
}

Value::operator float() const
{
    if (!IsNumeric())
        throw std::runtime_error("json mValueType==IsNumeric() required");

    return mFloatVal;
}

Value::operator double() const
{
    if (!IsNumeric())
        throw std::runtime_error("json mValueType==IsNumeric() required");

    return mDoubleVal;
}

Value::operator bool() const
{
    if (mValueType != BoolVal)
        throw std::runtime_error("json mValueType==BoolVal required");

    return mBoolVal;
}

Value::operator std::string() const
{
    if (mValueType != StringVal)
        throw std::runtime_error("json mValueType==StringVal required");

    return mStringVal;
}

Value::operator Object() const
{
    if (mValueType != ObjectVal)
        throw std::runtime_error("json mValueType==ObjectVal required");

    return mObjectVal;
}

Value::operator Array() const
{
    if (mValueType != ArrayVal)
        throw std::runtime_error("json mValueType==ArrayVal required");

    return mArrayVal;
}

/**
 * @brief Array::Array - массив переменных
 */
Array::Array()
{
}

Array::Array(const Array& a) : mValues(a.mValues)
{
}

Array& Array::operator =(const Array& a)
{
    if (&a == this)
        return *this;

    Clear();
    mValues = a.mValues;

    return *this;
}

Value& Array::operator [](size_t i)
{
    return mValues[i];
}

const Value& Array::operator [](size_t i) const
{
    return mValues[i];
}


Array::ValueVector::const_iterator Array::begin() const
{
    return mValues.begin();
}

Array::ValueVector::const_iterator Array::end() const
{
    return mValues.end();
}

Array::ValueVector::iterator Array::begin()
{
    return mValues.begin();
}

Array::ValueVector::iterator Array::end()
{
    return mValues.end();
}

void Array::push_back(const Value& v)
{
    mValues.push_back(v);
}

void Array::insert(size_t index, const Value& v)
{
    mValues.insert(mValues.begin() + index, v);
}

size_t Array::size() const
{
    return mValues.size();
}

void Array::Clear()
{
    mValues.clear();
}

Array::ValueVector::iterator Array::find(const Value& v)
{
    return std::find(mValues.begin(), mValues.end(), v);
}

Array::ValueVector::const_iterator Array::find(const Value& v) const
{
    return std::find(mValues.begin(), mValues.end(), v);
}

bool Array::HasValue(const Value& v) const
{
    return find(v) != end();
}

/**
 * @brief Object::Object - сущность объекта
 */
Object::Object()
{
}

Object::Object(const Object& obj) : mValues(obj.mValues)
{

}

Object& Object::operator =(const Object& obj)
{
    if (&obj == this)
        return *this;

    Clear();
    mValues = obj.mValues;

    return *this;
}

Value& Object::operator [](const std::string& key)
{
    return mValues[key];
}

const Value& Object::operator [](const std::string& key) const
{
    ValueMap::const_iterator it = mValues.find(key);
    return it->second;
}

Value& Object::operator [](const char* key)
{
    return mValues[key];
}

const Value& Object::operator [](const char* key) const
{
    ValueMap::const_iterator it = mValues.find(key);
    return it->second;
}

Object::ValueMap::const_iterator Object::begin() const
{
    return mValues.begin();
}

Object::ValueMap::const_iterator Object::end() const
{
    return mValues.end();
}

Object::ValueMap::iterator Object::begin()
{
    return mValues.begin();
}

Object::ValueMap::iterator Object::end()
{
    return mValues.end();
}

Object::ValueMap::iterator Object::find(const std::string& key)
{
    return mValues.find(key);
}

Object::ValueMap::const_iterator Object::find(const std::string& key) const
{
    return mValues.find(key);
}

bool Object::HasKey(const std::string& key) const
{
    return find(key) != end();
}

int Object::HasKeys(const std::vector<std::string>& keys) const
{
    for (size_t i = 0; i < keys.size(); i++)
    {
        if (!HasKey(keys[i]))
            return (int)i;
    }

    return -1;
}

int Object::HasKeys(const char** keys, int key_count) const
{
    for (int i = 0; i < key_count; i++)
        if (!HasKey(keys[i]))
            return i;

    return -1;
}

void Object::Clear()
{
    mValues.clear();
}


/**
 * Методы сервиса и конфигурации
 */

namespace json {

std::string SerializeArray(const Array& a);

std::string SerializeValue(const Value& v)
{
    std::string str;

    static const int BUFF_SZ = 500;
    char buff[BUFF_SZ];
    switch (v.GetType())
    {
    case IntVal			:
        snprintf(buff, BUFF_SZ, "%d", (int)v);
        str = buff;
        break;
    case FloatVal		:
        snprintf(buff, BUFF_SZ, "%f", (float)v);
        str = buff;
        break;
    case DoubleVal		:
        snprintf(buff, BUFF_SZ,
                 theConfig.getDoubleFormat(),
                 (double)v);
        str = buff;
        break;
    case BoolVal		:
        str = v ? "true" : "false";
        break;
    case NULLVal		:
        str = "null";
        break;
    case ObjectVal		:
        str = Serialize(v);
        break;
    case ArrayVal		:
        str = SerializeArray(v);
        break;
    case StringVal		:
        str = std::string("\"") + (std::string)v + std::string("\"");
        break;
    }

    return str;
}


std::string SerializeArray(const Array& a)
{
    std::string str = "[";

    bool first = true;
    for (size_t i = 0; i < a.size(); i++)
    {
        const Value& v = a[i];
        if (!first)
            str += std::string(",");

        str += SerializeValue(v);

        first = false;
    }

    str += "]";
    return str;
}
}

std::string json::Serialize(const Value& v)
{
    std::string str;

    bool first = true;

    if (v.GetType() == ObjectVal)
    {
        str = "{";
        Object obj = v.ToObject();
        for (Object::ValueMap::const_iterator it = obj.begin(); it != obj.end(); ++it)
        {
            if (!first)
                str += std::string(",");

            str += std::string("\"") + it->first + std::string("\":") + SerializeValue(it->second);
            first = false;
        }

        str += "}";
    }
    else if (v.GetType() == ArrayVal)
    {
        str = "[";
        Array a = v.ToArray();
        for (Array::ValueVector::const_iterator it = a.begin(); it != a.end(); ++it)
        {
            if (!first)
                str += std::string(",");

            str += SerializeValue(*it);
            first = false;
        }

        str += "]";

    }
    // иначе это недопустимый JSON, поскольку структура данных JSON должна быть массивом или объектом.
    // Мы вернем пустую строку.


    return str;
}

/**
 * Приватные функции
 */
static Value DeserializeArray(std::string& str, std::stack<StackDepthType>& depth_stack);
static Value DeserializeObj(const std::string& _str, std::stack<StackDepthType>& depth_stack);

static Value DeserializeInternal(const std::string& _str, std::stack<StackDepthType>& depth_stack)
{
    Value v;

    std::string str = Trim(_str);
    if (str[0] == '{')
    {
        // Error: Начинается с {, но не заканчивается
        if (str[str.length() - 1] != '}')
            return Value();

        depth_stack.push(InObject);
        v = DeserializeObj(str, depth_stack);
        if ((v.GetType() == NULLVal) || (depth_stack.top() != InObject))
            return v;

        depth_stack.pop();
    }
    else if (str[0] == '[')
    {
        // Error: Начинается с [но не заканчивается
        if (str[str.length() - 1] != ']')
            return Value();

        depth_stack.push(InArray);
        v = DeserializeArray(str, depth_stack);
        if ((v.GetType() == NULLVal) || (depth_stack.top() != InArray))
            return v;

        depth_stack.pop();
    }
    else
    {
        // Никогда не попадет сюда, если _str не является действительным JSON
        return Value();
    }

    return v;
}

static size_t GetEndOfArrayOrObj(const std::string& str, std::stack<StackDepthType>& depth_stack)
{
    size_t i = 1;
    bool in_quote = false;
    size_t original_count = depth_stack.size();

    for (; i < str.length(); i++)
    {
        if (str[i] == '\"')
        {
            if (str[i - 1] != '\\')
                in_quote = !in_quote;
        }
        else if (!in_quote)
        {
            if (str[i] == '[')
                depth_stack.push(InArray);
            else if (str[i] == '{')
                depth_stack.push(InObject);
            else if (str[i] == ']')
            {
                StackDepthType t = depth_stack.top();
                if (t != InArray)
                {
                    // ожидается закрытие массива, но вместо этого мы внутри объектного блока.
                    // Example problem: {]}
                    return std::string::npos;
                }

                size_t count = depth_stack.size();
                depth_stack.pop();
                if (count == original_count)
                    break;
            }
            else if (str[i] == '}')
            {
                StackDepthType t = depth_stack.top();
                if (t != InObject)
                {
                    // ожидается закрытие объекта, но вместо этого мы внутри массива.
                    // Example problem: [}]
                    return std::string::npos;
                }

                size_t count = depth_stack.size();
                depth_stack.pop();
                if (count == original_count)
                    break;
            }
        }
    }

    return i;
}

static std::string UnescapeJSONString(const std::string& str)
{
    std::string s = "";

    for (std::string::size_type i = 0; i < str.length(); i++)
    {
        char c = str[i];
        if ((c == '\\') && (i + 1 < str.length()))
        {
            int skip_ahead = 1;
            unsigned int hex;
            std::string hex_str;

            switch (str[i+1])
            {
            case '"' :
                s.push_back('\"');
                break;
            case '\\':
                s.push_back('\\');
                break;
            case '/' :
                s.push_back('/');
                break;
            case 't' :
                s.push_back('\t');
                break;
            case 'n' :
                s.push_back('\n');
                break;
            case 'r' :
                s.push_back('\r');
                break;
            case 'b' :
                s.push_back('\b');
                break;
            case 'f' :
                s.push_back('\f');
                break;
            case 'u' :
                skip_ahead = 5;
                hex_str = str.substr(i + 4, 2);
                hex = (unsigned int)std::strtoul(hex_str.c_str(), nullptr, 16);
                s.push_back((char)hex);
                break;

            default:
                break;
            }

            i += skip_ahead;
        }
        else
            s.push_back(c);
    }

    return Trim(s);
}

static Value DeserializeValue(std::string& str, bool* had_error, std::stack<StackDepthType>& depth_stack)
{
    Value v;

    *had_error = false;
    str = Trim(str);

    if (str.length() == 0)
        return v;

    if (str[0] == '[')
    {
        // Это значение представляет собой массив, определите его конец,
        // а затем десериализуйте массив
        depth_stack.push(InArray);
        size_t i = GetEndOfArrayOrObj(str, depth_stack);
        if (i == std::string::npos)
        {
            *had_error = true;
            return Value();
        }

        std::string array_str = str.substr(0, i + 1);
        v = Value(DeserializeArray(array_str, depth_stack));
        str = str.substr(i + 1, str.length());
    }
    else if (str[0] == '{')
    {
        // Это значение является объектом, определите его конец, а затем десериализуйте объект
        depth_stack.push(InObject);
        size_t i = GetEndOfArrayOrObj(str, depth_stack);

        if (i == std::string::npos)
        {
            *had_error = true;
            return Value();
        }

        std::string obj_str = str.substr(0, i + 1);

        v = Value(DeserializeInternal(obj_str, depth_stack));
        str = str.substr(i + 1, str.length());
    }
    else if (str[0] == '\"')
    {
        // Это значение - строка
        size_t end_quote = GetQuotePos(str, 1);
        if (end_quote == std::string::npos)
        {
            *had_error = true;
            return Value();
        }

        v = Value(UnescapeJSONString(str.substr(1, end_quote - 1)));
        str = str.substr(end_quote + 1, str.length());
    }
    else
    {
        // это не объект, строка или массив, поэтому это либо логическое значение, либо число, либо ноль.
        // Числа могут содержать показатель степени («е») или десятичную точку.
        bool has_dot = false;
        bool has_e = false;
        std::string temp_val;
        size_t i = 0;
        bool found_digit = false;
        bool found_first_valid_char = false;

        for (; i < str.length(); i++)
        {
            if (str[i] == '.')
            {
                if (!found_digit)
                {
                    // Согласно стандартам JSON, перед десятичной точкой должна быть цифра.
                    *had_error = true;
                    return Value();
                }

                has_dot = true;
            }
            else if ((str[i] == 'e') || (str[i] == 'E'))
            {
                if ((_stricmp(temp_val.c_str(), "fals") != 0) && (_stricmp(temp_val.c_str(), "tru") != 0))
                {
                    // это не логическое значение, проверьте правильность научной записи.
                    // Это также перехватит логические значения с дополнительными символами 'e',
                    // такими как falsee / truee
                    if (!found_digit)
                    {
                        // Согласно стандартам JSON, цифра должна предшествовать нотации 'e'.
                        *had_error = true;
                        return Value();
                    }
                    else if (has_e)
                    {
                        // несколько символов 'e' не допускаются
                        *had_error = true;
                        return Value();
                    }

                    has_e = true;
                }
            }
            else if (str[i] == ']')
            {
                if (depth_stack.empty() || (depth_stack.top() != InArray))
                {
                    *had_error = true;
                    return Value();
                }

                depth_stack.pop();
            }
            else if (str[i] == '}')
            {
                if (depth_stack.empty() || (depth_stack.top() != InObject))
                {
                    *had_error = true;
                    return Value();
                }

                depth_stack.pop();
            }
            else if (str[i] == ',')
                break;
            else if ((str[i] == '[') || (str[i] == '{'))
            {
                // ошибка, мы должны обрабатывать здесь не только массивы / объекты
                *had_error = true;
                return Value();
            }

            if (!std::isspace(static_cast<unsigned char>(str[i])))
            {
                if (std::isdigit(str[i]))
                    found_digit = true;

                found_first_valid_char = true;
                temp_val += str[i];
            }
        }

        // хранить все числа с плавающей запятой как двойные. Это также установит значения float и int.
        if (_stricmp(temp_val.c_str(), "true") == 0)
            v = Value(true);
        else if (_stricmp(temp_val.c_str(), "false") == 0)
            v = Value(false);
        else if (has_e || has_dot)
        {
            char* end_char;
            errno = 0;
            double d = strtod(temp_val.c_str(), &end_char);
            if ((errno != 0) || (*end_char != '\0'))
            {
                // invalid conversion or out of range
                *had_error = true;
                return Value();
            }

            v = Value(d);
        }
        else if (_stricmp(temp_val.c_str(), "null") == 0)
            v = Value();
        else
        {
            // Проверьте, не превышает ли значение размер int, и если да, сохраните его как double
            char* end_char;
            errno = 0;
            long int ival = strtol(temp_val.c_str(), &end_char, 10);
            if (*end_char != '\0')
            {
                // invalid character sequence, not a number
                *had_error = true;
                return Value();
            }
            else if ((errno == ERANGE) && ((ival == LONG_MAX) || (ival == LONG_MIN)))
            {
                // значение вне допустимого диапазона для длинного int, тогда должно быть double.
                // Посмотрим, сможем ли мы его правильно преобразовать.
                errno = 0;
                double dval = strtod(temp_val.c_str(), &end_char);
                if ((errno != 0) || (*end_char != '\0'))
                {
                    // error в конвертации или слишком большой для double
                    *had_error = true;
                    return Value();
                }

                v = Value(dval);
            }
            else if ((ival >= INT_MIN) && (ival <= INT_MAX))
            {
                // допустимый целочисленный диапазон
                v = Value((int)ival);
            }
            else
            {
                // вероятно, работает на очень старой ОС, поскольку этот блок подразумевает, что long не такого же размера, как int.
                // int гарантированно будет не менее 16 бит и long 32 бит ... однако в настоящее время они почти
                // всегда одинаковый размер 32 бита. Но возможно, кто-то запускает это на очень старой архитектуре
                // так что для корректности мы здесь ошибаемся
                *had_error = true;
                return Value();
            }
        }

        str = str.substr(i, str.length());
    }

    return v;
}

static Value DeserializeArray(std::string& str, std::stack<StackDepthType>& depth_stack)
{
    Array a;
    bool had_error = false;

    str = Trim(str);

    // Массивы начинаются и заканчиваются [], поэтому, если мы не найдем его, это ошибка.
    if ((str[0] == '[') && (str[str.length() - 1] == ']'))
        str = str.substr(1, str.length() - 2);
    else
        return Value();

    // извлечь все значения из массива (помните, значение также может быть массивом или объектом)
    while (str.length() > 0)
    {
        std::string tmp;

        size_t i = 0;
        for (; i < str.length(); i++)
        {
            // Если мы дойдем до объекта или массива, проанализируем его:
            if ((str[i] == '{') || (str[i] == '['))
            {
                Value v = DeserializeValue(str, &had_error, depth_stack);
                if (had_error)
                    return Value();

                if (v.GetType() != NULLVal)
                    a.push_back(v);

                break;
            }

            bool terminate_parsing = false;

            if ((str[i] == ',') || (str[i] == ']'))
                terminate_parsing = true;			// пометить до конца значения, проанализировать его в следующем блоке
            else
            {
                // продолжаем собирать символы, чтобы увеличить значение
                tmp += str[i];
                if  (i == str.length() - 1)
                    terminate_parsing = true; // конец строки, закончить разбор
            }

            if (terminate_parsing)
            {
                Value v = DeserializeValue(tmp, &had_error, depth_stack);
                if (had_error)
                    return Value();

                if (v.GetType() != NULLVal)
                    a.push_back(v);

                str = str.substr(i + 1, str.length());
                break;
            }
        }
    }

    return a;
}

static Value DeserializeObj(const std::string& _str, std::stack<StackDepthType>& depth_stack)
{
    Object obj;

    std::string str = Trim(_str);

    // Объекты начинаются и заканчиваются на {}, поэтому, если мы не находим пару, это ошибка.
    if ((str[0] != '{') && (str[str.length() - 1] != '}'))
        return Value();
    else
        str = str.substr(1, str.length() - 2);

    // Получить все пары ключ / значение в этом объекте ...
    while (str.length() > 0)
    {
        // Get the key name
        size_t start_quote_idx = GetQuotePos(str);
        size_t end_quote_idx = GetQuotePos(str, start_quote_idx + 1);
        size_t colon_idx = str.find(':', end_quote_idx);

        if ((start_quote_idx == std::string::npos) || (end_quote_idx == std::string::npos) || (colon_idx == std::string::npos))
            return Value();	// не могу найти ключевое имя

        std::string key = str.substr(start_quote_idx + 1, end_quote_idx - start_quote_idx - 1);
        if (key.length() == 0)
            return Value();

        bool had_error = false;
        str = str.substr(colon_idx + 1, str.length());

        // У нас есть ключ, теперь извлекаем значение из строки
        obj[key] = DeserializeValue(str, &had_error, depth_stack);
        if (had_error)
            return Value();
    }

    return obj;
}

Value json::Deserialize(const std::string &str)
{
    std::stack<StackDepthType> depth_stack;
    return DeserializeInternal(str, depth_stack);
}
