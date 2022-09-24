

func square(var n) return n*n


func fib1(var n)
    if n < 0 return 0
    if n < 2 return 1
    return fib1(n-1) + fib1(n-2)


func fib2(var n)
    if n < 2 return 1
    var f1, f2, r = 1
    for n > 0
        r = f1 + f2
        f1,f2 = f2,r
        n -= 1
    return r


func main()
    var i = 2
    i = square(6)
    return i




// (init)
// loop:
// (condition)
// jmp 0 end
//   (block)
//   (inc)
// jmp 2 loop
// end: