#pragma once

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

// Source - https://stackoverflow.com/q
// Posted by Dmitriano, modified by community. See post 'Timeline' for change history
// Retrieved 2026-01-04, License - CC BY-SA 3.0

template<typename TLambda>
class OnScopeExit
{
public:

    OnScopeExit(const TLambda & functor, bool isEngaged = true) : _functor(functor), _isEngaged(isEngaged)
    {
    }

    OnScopeExit(const OnScopeExit &) = delete;
    OnScopeExit(scope_guard && other) = delete;

    OnScopeExit & operator=(const OnScopeExit &) = delete;
    OnScopeExit & operator=(OnScopeExit && other) = delete;

    ~OnScopeExit()
    {
        if (!_isEngaged)
            return;

        _functor();
    }

private:
    TLambda _functor;
    bool _isEngaged;
};

