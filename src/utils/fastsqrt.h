/* A very fast function to calculate the approximate inverse square root of a
 * floating point value and a helper function that uses it for getting the
 * normal squareroot. For an explanation of the inverse squareroot function
 * read:
 * http://www.math.purdue.edu/~clomont/Math/Papers/2003/InvSqrt.pdf
 *
 * Unfortunately the original creator of this function seems to be unknown.
 */

float fastInvSqrt(float x)
{
    float xhalf = 0.5f*x;
    int i = *(int*)&x;
    i = 0x5f375a86- (i>>1);
    x = *(float*)&i;
    x = x*(1.5f-xhalf*x*x);
    return x;
}

float fastSqrt(float x)
{
    return 1.0f/fastInvSqrt(x);
}
