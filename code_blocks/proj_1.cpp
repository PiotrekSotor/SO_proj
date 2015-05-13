#include<ncurses.h>
//#include<iostream>
#include<thread>
#include<mutex>
#include<condition_variable>
#include<vector>

int l_klient;

std::mutex mx_zasoby;
std::condition_variable cv_zasoby;
int zasoby;

std::vector<std::mutex>mx_klient;
std::vector<std::condition_variable>cv_klient;
std::vector<int>vec_aktywni_klienci;
int endFlag;


void klient(int id)
{
    while (1)
    {
        std::unique_lock<std::mutex>lock(mx_klient[id]);
        cv_klient[id].wait(lock,[](int id){return vec_aktywni_klienci[id] == 1 || endFlag == 1;});
        if (endFlag == 1)
            return;

        printw("klient %d",id);
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

    }
}
void control()
{
    int counter =0;
    while (1)
    {
        timeout(20);
        char c = getch();
        if (c == 'a')
        {
            if (counter <= l_klient - 1)
            {
                vec_aktywni_klienci[counter]=1;
                counter++;
            }
            else
                printw("Brak wolnych klientow\n");
        }
        else if (c == 'z')
        {
            if (counter > 0)
            {
                vec_aktywni_klienci[counter]=0;
                counter--;
            }
            else
                printw("Brak aktywnych klientow\n");
        }
        else if (c== 'q')
        {
            endFlag = 1;
            break;
        }


    }
}



int main(int argc, char** argv)
{
    initscr();
    l_klient = atoi(argv[1]);
    mx_klient = std::vector<std::mutex>(atoi(argv[1]));
    cv_klient = std::vector<std::condition_variable>(atoi(argv[1]));
    vec_aktywni_klienci = std::vector<int>(atoi(argv[1]));
    for (int i=0;i<atoi(argv[1]);++i)
        vec_aktywni_klienci[i]=0;
    std::vector<std::thread> vec_klient_thread;
    for (int i =0; i< atoi(argv[1]); ++i)
        vec_klient_thread.push_back(std::thread(klient,i));
    std::thread control_thread(control);



    endwin();
    return 0;
}
