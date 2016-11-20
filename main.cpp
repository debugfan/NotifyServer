#include <iostream>
#include "send_mail.h"
#include <stdio.h>

using namespace std;

int main()
{
    send_mail("smtp.163.com", 1, "user@163.com", "pass",
        NULL, "receiver@163.com", NULL,
        "test subject", "test message");

    return 0;
}
