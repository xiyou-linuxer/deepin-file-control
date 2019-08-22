#ifndef _NONCOPYABLE_H
#define _NONCOPYABLE_H

class Noncopyable {
public:
    Noncopyable() {  }
    ~Noncopyable() {  }
private:
    Noncopyable(const Noncopyable&);
    const Noncopyable& operator=(const Noncopyable&); 
};

#endif // _NONCOPYABLE_H
