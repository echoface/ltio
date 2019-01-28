#include <iostream>
#include <cstring>
#include <chrono>
#include <ImageMagick-6/Magick++.h>

using namespace std;
using namespace Magick;

int main(int argc, char **argv)
{
    InitializeMagick(*argv);

    Image svg;
    Image bkg;

    try
    {

       chrono::time_point<std::chrono::system_clock> now = chrono::system_clock::now();
       auto duration = now.time_since_epoch();
       auto millis = chrono::duration_cast<std::chrono::milliseconds>(duration).count();

       cout << " start:" << millis << endl;
       // Read a file into image object
       svg.backgroundColor("None");
       svg.read("android.svg");


        //bkg.read("r-01.bmp");


        //bkg.composite(svg, 0, 0, CompositeOperator::OverCompositeOp);
        //bkg.quality(80);

        // Write the image to a file
        //bkg.write("01_r.jpg");
        svg.write("01.png");
        {
          chrono::time_point<std::chrono::system_clock> now = chrono::system_clock::now();
          auto duration = now.time_since_epoch();
          auto millis = chrono::duration_cast<std::chrono::milliseconds>(duration).count();
          cout << " start:" << millis << endl;
        }
    } catch (Exception &error_) {
        cout << "Caught exception: " << error_.what() << endl;
        return 1;
    }
    return 0;
}
