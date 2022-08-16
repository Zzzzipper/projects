#ifndef __JSON_H__
#define __JSON_H__

#include <vector>
#include <map>
#include <string>
#include <stdexcept>


// ПОЖАЛУЙСТА, ПРОСМОТРЕТЬ README ДЛЯ ИНФОРМАЦИИ ПО ИСПОЛЬЗОВАНИЮ И ПРИМЕРОВ.
// Комментарии будут сведены к минимуму, чтобы уменьшить беспорядок.
namespace json
{
enum ValueType
{
    NULLVal,
    StringVal,
    IntVal,
    FloatVal,
    DoubleVal,
    ObjectVal,
    ArrayVal,
    BoolVal
};

class Value;

// Представляет объект JSON, имеющий форму {строка: значение, строка: значение, ...}
// Где строка - это "ключевое" имя и
// формы "" или "символы". Значение может быть одним из: строка, число, объект, массив,
// логическое значение, ноль.
class Object
{
public:

    // Это тип, используемый для хранения пар ключ / значение. Если вы хотите получить
    // итератор для этого класса для перебора его членов,
    // используйте этот.
    // Например: Object::ValueMap::iterator my_iterator;
    typedef std::map<std::string, Value> ValueMap;

protected:
    ValueMap mValues;

public:

    Object();
    Object(const Object& obj);

    Object& operator =(const Object& obj);

    friend bool operator ==(const Object& lhs, const Object& rhs);
    inline friend bool operator !=(const Object& lhs, const Object& rhs) {
        return !(lhs == rhs);
    }
    friend bool operator <(const Object& lhs, const Object& rhs);
    inline friend bool operator >(const Object& lhs, const Object& rhs) {
        return operator<(rhs, lhs);
    }
    inline friend bool operator <=(const Object& lhs, const Object& rhs) {
        return !operator>(lhs, rhs);
    }
    inline friend bool operator >=(const Object& lhs, const Object& rhs) {
        return !operator<(lhs, rhs);
    }

    // Как и в случае с std :: map, вы можете получить значение ключа с помощью оператора индекса.
    // Вы также можете используйте это, чтобы вставить значение, если оно не существует, или
    // перезаписать его, если оно есть. Пример:
    // Значение my_val = my_object ["какое-то имя ключа"];
    // my_object ["какое-то имя ключа"] = "перезапись значения этим новым строковым значением";
    // my_object ["имя нового ключа"] = "вставляется новый ключ";
    Value& operator [](const std::string& key);
    const Value& operator [](const std::string& key) const;
    Value& operator [](const char* key);
    const Value& operator [](const char* key) const;

    ValueMap::const_iterator begin() const;
    ValueMap::const_iterator end() const;
    ValueMap::iterator begin();
    ValueMap::iterator end();

    // Поиск вернет end(), если ключ не может быть найден, как это делает std::map.
    // ->first будет ключ (std::string),
    // ->second будет Value.
    ValueMap::iterator find(const std::string& key);
    ValueMap::const_iterator find(const std::string& key) const;

    // Удобная обертка для поиска ключа
    bool HasKey(const std::string& key) const;

    // Проверяет, содержит ли объект все ключи в массиве. Если это так, возвращает -1.
    // Если нет, возвращает индекс первого ключа, который не смог найти.
    int HasKeys(const std::vector<std::string>& keys) const;
    int HasKeys(const char* keys[], int key_count) const;

    // Удаляет все значения и возвращает состояние по умолчанию
    void Clear();

    size_t size() const {return mValues.size();}

};

// Представляет массив JSON, имеющий форму [value, value, ...]
// где value может быть одним из: string, number, object, array, boolean, null
class Array
{
public:

    // Это тип, используемый для хранения значений. Если вы хотите получить итератор для этого класса,
    // чтобы перебирать его члены, использовать этот.
    // Например: Array::ValueVector::iterator my_array_iterator;
    typedef std::vector<Value> ValueVector;

protected:

    ValueVector mValues;

public:

    Array();
    Array(const Array& a);

    Array& operator =(const Array& a);

    friend bool operator ==(const Array& lhs, const Array& rhs);
    inline friend bool operator !=(const Array& lhs, const Array& rhs) {
        return !(lhs == rhs);
    }
    friend bool operator <(const Array& lhs, const Array& rhs);
    inline friend bool operator >(const Array& lhs, const Array& rhs) {
        return operator<(rhs, lhs);
    }
    inline friend bool operator <=(const Array& lhs, const Array& rhs) {
        return !operator>(lhs, rhs);
    }
    inline friend bool operator >=(const Array& lhs, const Array& rhs) {
        return !operator<(lhs, rhs);
    }

    Value& operator[] (size_t i);
    const Value& operator[] (size_t i) const;

    ValueVector::const_iterator begin() const;
    ValueVector::const_iterator end() const;
    ValueVector::iterator begin();
    ValueVector::iterator end();

    // Просто удобная оболочка для выполнения std::find (Array::begin(), Array::end(), Value)
    ValueVector::iterator find(const Value& v);
    ValueVector::const_iterator find(const Value& v) const;

    // Удобная оболочка для проверки наличия значения в массиве
    bool HasValue(const Value& v) const;

    // Удаляет все значения и возвращает состояние по умолчанию
    void Clear();

    void push_back(const Value& v);
    void insert(size_t index, const Value& v);
    size_t size() const;
};

// Представляет значение JSON, которое может иметь одно из следующих значений:
// string, number, object, array, boolean, null.

class Value
{
protected:

    ValueType   mValueType;
    int         mIntVal;
    int64_t     mInt64Val;
    uint64_t    mUint64Val;
    float       mFloatVal;
    double      mDoubleVal;
    std::string mStringVal;
    Object      mObjectVal;
    Array       mArrayVal;
    bool        mBoolVal;

public:

    Value()
        : mValueType(NULLVal)
        , mIntVal(0)
        , mFloatVal(0)
        , mDoubleVal(0)
        , mBoolVal(false)
    {}
    Value(int v)
        : mValueType(IntVal)
        , mIntVal(v)
        , mFloatVal((float)v)
        , mDoubleVal((double)v)
        , mBoolVal(false)
    {}
    Value(float v)
        : mValueType(FloatVal)
        , mIntVal((int)v)
        , mFloatVal(v)
        , mDoubleVal((double)v)
        , mBoolVal(false) {}
    Value(double v)
        : mValueType(DoubleVal)
        , mIntVal((int)v)
        , mFloatVal((float)v)
        , mDoubleVal(v)
        , mBoolVal(false) {}
    Value(const std::string& v)
        : mValueType(StringVal)
        , mIntVal()
        , mFloatVal()
        , mDoubleVal()
        , mStringVal(v)
        , mBoolVal(false) {}
    Value(const char* v)
        : mValueType(StringVal)
        , mIntVal()
        , mFloatVal()
        , mDoubleVal()
        , mStringVal(v)
        , mBoolVal(false) {}
    Value(const Object& v)
        : mValueType(ObjectVal)
        , mIntVal()
        , mFloatVal()
        , mDoubleVal()
        , mObjectVal(v)
        , mBoolVal(false) {}
    Value(const Array& v)
        : mValueType(ArrayVal)
        , mIntVal()
        , mFloatVal()
        , mDoubleVal()
        , mArrayVal(v)
        , mBoolVal(false) {}
    Value(bool v)
        : mValueType(BoolVal)
        , mIntVal()
        , mFloatVal()
        , mDoubleVal()
        , mBoolVal(v) {}
    Value(const Value& v);

    // Используйте эту функцию, чтобы определить базовый тип, который представляет этот класс Value. Это будет один из
    // ValueType описатель, как определено в верхней части этого файла.
    ValueType GetType() const {return mValueType;}

    // Удобный метод, который проверяет, является ли этот тип int/double/float
    bool IsNumeric() const {
        return (mValueType == IntVal) || (mValueType == DoubleVal) || (mValueType == FloatVal);
    }

    Value& operator =(const Value& v);

    friend bool operator ==(const Value& lhs, const Value& rhs);
    inline friend bool operator !=(const Value& lhs, const Value& rhs) {
        return !(lhs == rhs);
    }
    friend bool operator <(const Value& lhs, const Value& rhs);
    inline friend bool operator >(const Value& lhs, const Value& rhs) {
        return operator<(rhs, lhs);
    }
    inline friend bool operator <=(const Value& lhs, const Value& rhs) {
        return !operator>(lhs, rhs);
    }
    inline friend bool operator >=(const Value& lhs, const Value& rhs) {
        return !operator<(lhs, rhs);
    }


    // Если это значение представляет объект или массив, вы можете использовать оператор индексации []
    // точно так же, как с родными классами json :: Array или json :: Object.
    // ВЫБРАСЫВАЕТ std :: runtime_error, ЕСЛИ НЕ МАССИВ ИЛИ ОБЪЕКТ.
    Value& operator [](size_t idx);
    const Value& operator [](size_t idx) const;
    Value& operator [](const std::string& key);
    const Value& operator [](const std::string& key) const;
    Value& operator [](const char* key);
    const Value& operator [](const char* key) const;

    // Если это значение представляет объект, эти методы позволяют проверить, является ли один ключ или массив
    // ключей содержатся внутри него.
    // ВЫБРАСЫВАЕТ std :: runtime_error ЕСЛИ НЕ ОБЪЕКТ.
    bool HasKey(const std::string& key) const;
    int  HasKeys(const std::vector<std::string>& keys) const;
    int  HasKeys(const char* keys[], int key_count) const;


    // преобразования в тип ** вызовут ошибку std::runtime_error, если она недопустима, с соответствующим сообщением об ошибке **
    int      ToInt() const;
    int      ToInt32() const;
    float    ToFloat() const;
    double   ToDouble() const;
    bool     ToBool() const;
    const std::string&	ToString() const;
    Object   ToObject() const;
    Array ToArray() const;

    // Простой доступ к управлению массивом -- С ОСТОРОЖНОСТЬЮ!
    Array& array();

    // Эти версии делают то же самое, что и выше, но возвращают указанное вами значение по умолчанию в случае ошибки и,
    // таким образом, ** не ** генерируют исключение.
    int	ToInt(int def) const	{
        return IsNumeric() ? mIntVal : def;
    }
    float  ToFloat(float def) const {
        return IsNumeric() ? mFloatVal : def;
    }
    double ToDouble(double def) const {
        return IsNumeric() ? mDoubleVal : def;
    }
    bool   ToBool(bool def) const {
        return (mValueType == BoolVal) ? mBoolVal : def;
    }
    const std::string&	ToString(const std::string& def) const	{
        return (mValueType == StringVal) ? mStringVal : def;
    }


    // Обратите внимание, что в соответствии с правилами C ++ неявное приведение Value к std :: string не работает.
    // Это потому, что он также может использовать операторы int/float/double/bool. Итак, чтобы назначить
    // Значение для std::string, которое вы можете сделать:
    // my_string = (std :: string) my_value
    // Или теперь можете сделать:
    // my_string = my_value.ToString ();
    //
    operator int() const;
    operator float() const;
    operator double() const;
    operator bool() const;
    operator std::string() const;
    operator Object() const;
    operator Array() const;

    // Возвращает 1 для всего, что не является Array/ObjectVal
    size_t size() const;

    // Сбрасывает в состояние по умолчанию, также известное как NULLVal
    void Clear();

};

// Преобразует объект JSON или экземпляр массива в строку JSON, представляющую его. ВОЗВРАЩАЕТ ПУСТУЮ СТРОКУ ПРИ ОШИБКЕ.
// Согласно спецификации JSON, структура данных JSON должна быть массивом или объектом. Таким образом, вы должны
// передать на вход либо json::Array, json::Object или json::Value, у которого в качестве базового типа есть массив
// или объект.
std::string Serialize(const Value& obj);

// Если есть ошибка, Value будет NULLVal. Передайте допустимую строку JSON (например, возвращенную из Serialize или полученную
// в другом месте), чтобы получить взамен значение, представляющее структуру JSON. Проверьте тип значения, вызвав GetType ().
// Это будет ObjectVal или ArrayVal (или NULLVal, если JSON недействителен). Класс Value содержит оператор [] для индексации в
// если базовый тип является объектом или массивом. Вы можете, если хотите, создать объект или массив из возвращенного значения
// этим методом, просто передав его в конструктор.
Value Deserialize(const std::string& str);


inline bool operator ==(const Object& lhs, const Object& rhs)
{
    return lhs.mValues == rhs.mValues;
}

inline bool operator <(const Object& lhs, const Object& rhs)
{
    return lhs.mValues < rhs.mValues;
}

inline bool operator ==(const Array& lhs, const Array& rhs)
{
    return lhs.mValues == rhs.mValues;
}

inline bool operator <(const Array& lhs, const Array& rhs)
{
    return lhs.mValues < rhs.mValues;
}

/* При сравнении разных числовых типов этот метод работает так же, как если бы вы
 * самостоятельно сравнивал различные числовые типы. Таким образом, он работает так же, как если бы вы, например, сделали это:
 * int a = 1;
 * float b = 1.1f;
 * bool equivalent = a == b;
 * Та же логика применима и к другим операторам сравнения.
 */
inline bool operator ==(const Value& lhs, const Value& rhs)
{
    if ((lhs.mValueType != rhs.mValueType) && !lhs.IsNumeric() && !rhs.IsNumeric())
        return false;

    switch (lhs.mValueType)
    {
    case StringVal:
        return lhs.mStringVal == rhs.mStringVal;

    case IntVal:
        if (rhs.GetType() == FloatVal)
            return lhs.mIntVal == rhs.mFloatVal;
        else if (rhs.GetType() == DoubleVal)
            return lhs.mIntVal == rhs.mDoubleVal;
        else if (rhs.GetType() == IntVal)
            return lhs.mIntVal == rhs.mIntVal;
        else
            return false;

    case FloatVal:
        if (rhs.GetType() == FloatVal)
            return lhs.mFloatVal == rhs.mFloatVal;
        else if (rhs.GetType() == DoubleVal)
            return lhs.mFloatVal == rhs.mDoubleVal;
        else if (rhs.GetType() == IntVal)
            return lhs.mFloatVal == rhs.mIntVal;
        else
            return false;

    case DoubleVal:
        if (rhs.GetType() == FloatVal)
            return lhs.mDoubleVal == rhs.mFloatVal;
        else if (rhs.GetType() == DoubleVal)
            return lhs.mDoubleVal == rhs.mDoubleVal;
        else if (rhs.GetType() == IntVal)
            return lhs.mDoubleVal == rhs.mIntVal;
        else
            return false;

    case BoolVal:
        return lhs.mBoolVal == rhs.mBoolVal;

    case ObjectVal:
        return lhs.mObjectVal == rhs.mObjectVal;

    case ArrayVal:
        return lhs.mArrayVal == rhs.mArrayVal;

    default:
        return true;
    }
}

inline bool operator <(const Value& lhs, const Value& rhs)
{
    if ((lhs.mValueType != rhs.mValueType) && !lhs.IsNumeric() && !rhs.IsNumeric())
        return false;

    switch (lhs.mValueType)
    {
    case StringVal:
        return lhs.mStringVal < rhs.mStringVal;

    case IntVal:
        if (rhs.GetType() == FloatVal)
            return lhs.mIntVal < rhs.mFloatVal;
        else if (rhs.GetType() == DoubleVal)
            return lhs.mIntVal < rhs.mDoubleVal;
        else if (rhs.GetType() == IntVal)
            return lhs.mIntVal < rhs.mIntVal;
        else
            return false;

    case FloatVal:
        if (rhs.GetType() == FloatVal)
            return lhs.mFloatVal < rhs.mFloatVal;
        else if (rhs.GetType() == DoubleVal)
            return lhs.mFloatVal < rhs.mDoubleVal;
        else if (rhs.GetType() == IntVal)
            return lhs.mFloatVal < rhs.mIntVal;
        else
            return false;

    case DoubleVal:
        if (rhs.GetType() == FloatVal)
            return lhs.mDoubleVal < rhs.mFloatVal;
        else if (rhs.GetType() == DoubleVal)
            return lhs.mDoubleVal < rhs.mDoubleVal;
        else if (rhs.GetType() == IntVal)
            return lhs.mDoubleVal < rhs.mIntVal;
        else
            return false;

    case BoolVal:
        return lhs.mBoolVal < rhs.mBoolVal;

    case ObjectVal:
        return lhs.mObjectVal < rhs.mObjectVal;

    case ArrayVal:
        return lhs.mArrayVal < rhs.mArrayVal;

    default:
        return true;
    }
}

class cConfig
{
public:
    cConfig()
    {
        // Двойной формат по умолчанию показывает полную точность
        setDoubleFormat();
    }

    /** Изменить формат, используемый для вывода double's
     * @param [in] fmt новый формат, который будет использоваться, по умолчанию "% f"
     * например, json::theConfig.setDoubleFormat("% 1f");
     * Мотивация: иногда использование полной точности
     * ненужное и расходует пропускную способность.
     */
    void setDoubleFormat( const std::string& fmt = "%f" )
    {
        myDoubleFormat = fmt;
    }

    const char* getDoubleFormat()
    {
        return myDoubleFormat.c_str();
    }
private:
    std::string myDoubleFormat;
};

extern cConfig theConfig;
}

#endif //__JSON_H__
