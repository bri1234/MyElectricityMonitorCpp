#include "Json.h"
/*
Copyright (C) 2025  Torsten Brischalle
email: torsten@brischalle.de
web: http://www.aaabbb.de

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to
deal in the Software without restriction, including without limitation the
rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
IN THE SOFTWARE.
*/

#include "Json.h"

#include <stdexcept>

using namespace std;

Json::Json()
: _jsonRoot(nullptr)
{
}

Json::~Json()
{
    FreeJsonRoot();
}

void Json::LoadFromFile(const std::string & filename)
{
    FreeJsonRoot();

    _jsonRoot = json_object_from_file(filename.c_str());
    if (!_jsonRoot)
        throw Error("Failed to load JSON from file: " + filename);
}

void Json::FreeJsonRoot()
{
    if (!_jsonRoot)

    json_object_put(_jsonRoot);
    _jsonRoot = nullptr;
}

json_object * Json::GetRootObject() const
{
    if (!_jsonRoot)
        throw Error("JSON root object is null.");
        
    return _jsonRoot;
}
