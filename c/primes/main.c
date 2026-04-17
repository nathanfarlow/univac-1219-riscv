#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

uint32_t addmod(const uint32_t a, const uint32_t b, const uint32_t n)
{
    return (a + b) > n ? (a + b - n) : (a + b);
}

uint32_t multmod_loop(const uint32_t a, uint32_t b, const uint32_t n)
{
    uint32_t out = 0;

    for (int i = 0; i < 32; i++)
    {
        out = addmod(out, out, n);

        if (b & (1 << 31))
            out = addmod(out, a, n);

        b <<= 1;
    }

    return out;
}

uint32_t multmod(const uint32_t a, uint32_t b, const uint32_t n)
{
    uint32_t out = 0;

    return ((a % n) * (b % n)) % n;
}

uint32_t powmod(const uint32_t a, uint32_t b, const uint32_t n)
{
    uint32_t out = 1;

    for (int i = 0; i < 32; i++)
    {
        out = multmod(out, out, n);

        if (b & (1 << 31))
            out = multmod(out, a, n);

        b <<= 1;
    }

    return out;
}

bool isprime(uint32_t n)
{
    uint32_t test_nums[3] = {2, 7, 61};
    // uint32_t test_nums[12] = {2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37};

    uint32_t s = 0;
    uint32_t d = n - 1;

    while ((d & 1) == 0)
    {
        d >>= 1;
        s += 1;
    }

    for (int i = 0; i < 3; i++)
    {
        uint32_t a = test_nums[i];
        if (a >= n)
            continue;

        uint32_t x = powmod(a, d, n);

        uint32_t y = 0;
        for (int j = 0; j < s; j++)
        {
            y = multmod(x, x, n);

            if (y == 1 && x != 1 && x != (n - 1))
                return false;

            x = y;
        }

        if (y != 1)
            return false;
    }

    return true;
}

int main()
{
    uint8_t column = 0;

    for (uint32_t i = 2; i < 1000; i++)
    {
        if (isprime(i))
        {
            printf("%u, ", i);

            if (i < 10)
            {
                column += 3;
            }
            else if (i < 100)
            {
                column += 4;
            }
            else if (i < 1000)
            {
                column += 5;
            }
        }

        if (column >= 68)
        {
            printf("\n");
            column = 0;
        }
    }
}
