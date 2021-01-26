# The Silk Language
Silk is a lightweight cross-platform programming language which supports procedural programming and object-oriented programming. 

- It's simple, you can learn the entire syntax in 30 minutes.
- It's small, the compiled binary is only 600K without any dependencies.
- It's powerful, it can be integrated into any Text/HTML as server-side language. It's easy to extend Silk with libraries written in other languages.

Silk example:
```
/* 
The function to caculate fibonacci sequence 
parameter: n
return: fibonacci sequence 
*/
func Fibonacci(n)
{
    if (n == 0)
        return 0;
    else if (n <= 2)
        return 1;
    else
        return Fibonacci(n-1)+Fibonacci(n-2);//Recursion
}

main()
{
    //print the top 20 fibonacci sequence
    for(i=0;i<20;i++)
        print(Fibonacci(i));
}
```
## Building Silk
- **Windows** platform: 
  Download source code at GitHub, and extract it into any folder, open the solution file Silk.sln with Microsoft Visual Studio 2013 or higher to build the Silk program.

- **Linux** platform: 
  Download source code at GitHub, and extract it into any folder, please ensure you have installed gcc and g++, and run build.sh to build the Silk program.

- **MacOS** platform: 
  Download source code at GitHub, and extract it into any folder, please ensure you have installed XCode, and open the project file Silk.xcodeproj to build the Silk program.

This is the GitHub repository of Silk source code. Please visit Silk website [Silklang.org](https://silklang.org/) for details. 
