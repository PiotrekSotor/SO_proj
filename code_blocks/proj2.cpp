#include<ncurses.h>
#include<time.h>
#include<iostream>
#include<thread>
#include<vector>
#include<queue>
#include<mutex>
#include<condition_variable>

std::mutex mx_zasoby;
std::condition_variable cv_zasoby;
int store;
std::mutex mx_magazyn;
std::condition_variable cv_magazyn;
int warehouse;

int max_warehouse;
int min_warehouse;

int max_store = 10;
int min_store = 0;

int l_client;
int l_client_in_store;
int l_client_in_queue;
int l_client_shopping;

std::mutex *vec_mx_client;
std::condition_variable *vec_cv_client;
int* vec_active_client;

int l_kasjer;
std::mutex *vec_mx_kasjer;
std::condition_variable *vec_cv_kasjer;
int* vec_active_kasjer;

int l_wykladowca;
std::mutex *vec_mx_wykladowca;
std::condition_variable *vec_cv_wykladowca;
int* vec_active_wykladowca;

int l_dostawcy;


int l_pracownicy;

//liczba kas = liczba kasjerow
std::mutex mx_queue; // watek kierownika sklepu musi byc w stanie, klient musi miec czas by sprawdzic ktora kolejka jest najkrotsza
int* vec_active_queue;
int* vec_client_in_queue;
std::queue<int> *vec_queue;

int endFlag;
int endFlagDisplay;

int l_aktywnych_klientow=0;
int l_aktywnych_kasjerow=0;
int l_aktywnych_wykladowcow=0;




void notify_all_wykladowcy();
void notify_all_kasjerzy();
void notify_all_clients();


void consumer(int id)
{
	int counter = 10;
	while (1)
	{
        std::unique_lock<std::mutex> lock_1(vec_mx_client[id]);
        vec_cv_client[id].wait(lock_1,[&id](){return vec_active_client[id] == 1 || endFlag == 1;});

        if (endFlag == 1)
            break;

		//printw("klient %d wchodzi do sklepu\n",id);

		l_client_in_store++;
		l_client_shopping++;
		int  time = rand()%2000 + 1000;


		std::unique_lock<std::mutex> lock_2(mx_zasoby);
		cv_zasoby.wait(lock_2, []{return store > min_store || endFlag == 1;});
		if (endFlag == 1)
			break;
		store--;
		lock_2.unlock();
        cv_zasoby.notify_all();
        std::this_thread::sleep_for(std::chrono::milliseconds(time));

		int min_queue_nr=-1;
		do{
            //printw("lol\n");
            std::this_thread::sleep_for(std::chrono::milliseconds(25));

            if (endFlag == 1)
                return;
            std::unique_lock<std::mutex> lock(mx_queue);
            for (int i=0 ;i < l_kasjer; ++i)
            {
                //printw("lol2  vec_active_queue[%d] = %d\n",i,vec_active_queue[i]);
                std::this_thread::sleep_for(std::chrono::milliseconds(25));

                if (vec_active_queue[i] == 1)
                {
                    min_queue_nr = i;
                        break;
                }
            }
            for (int i=0 ;i < l_kasjer; ++i)
            {
                //printw("lol3  vec_active_queue[%d] = %d\tmin_queue_nr = %d\n",i,vec_active_queue[i],min_queue_nr);
                std::this_thread::sleep_for(std::chrono::milliseconds(25));

                if (vec_active_queue[i] == 1)
                    if (vec_queue[i].size()<vec_queue[min_queue_nr].size())
                        min_queue_nr = i;
            }
            //lock.unlock();
            //std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        while (min_queue_nr == -1 && endFlag == 0);
        if (endFlag ==1)
            break;
		//printw("klient %d wybral kolejke %d\n",id,min_queue_nr);
        std::this_thread::sleep_for(std::chrono::milliseconds(25));

		vec_queue[min_queue_nr].push(id);
		vec_client_in_queue[id] = 1;
		//stanie w kolejce
		l_client_shopping--;
		l_client_in_queue++;
		std::mutex mx_temp;
		std::unique_lock<std::mutex> lock_waiting_in_queue(mx_temp);
        vec_cv_client[id].wait(lock_waiting_in_queue,[&id](){return vec_client_in_queue[id] == 0 || endFlag == 1;});
        l_client_in_queue--;
		//wychodzi ze sklepu
		std::this_thread::sleep_for(std::chrono::milliseconds(20));
		//printw("klient %d wychodzi z sklepu\n",id);
        --l_client_in_store;
		time = rand()%2000 + 1000;
		std::this_thread::sleep_for(std::chrono::milliseconds(time));

	}
	printw("koniec klienta %d\n",id);
}

void kasjer(int id)
{


	while (1)
	{

		std::unique_lock<std::mutex> lock_1(vec_mx_kasjer[id]);
        vec_cv_kasjer[id].wait(lock_1,[&id](){return vec_active_kasjer[id] == 1 || endFlag == 1;});



        if (endFlag == 1)
            break;

		//printw("kasjer %d\n",id);
        int  time = rand()%500 + 1000;
		std::this_thread::sleep_for(std::chrono::milliseconds(time));

		while (vec_queue[id].empty() == false)
		{
            int  time = rand()%2000 + 1000;
            std::this_thread::sleep_for(std::chrono::milliseconds(time));
            std::unique_lock<std::mutex> lock_2(mx_queue);
            int klient_id = vec_queue[id].front();
            vec_queue[id].pop();
            lock_2.unlock();
            vec_client_in_queue[klient_id] = 0;
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            vec_cv_client[klient_id].notify_all();
		}
	}
	printw("kasjer %d\t KONIEC\n",id);
}

void wykladowca(int id)
{


	while (1)
	{

		std::unique_lock<std::mutex> lock_1(vec_mx_wykladowca[id]);
        vec_cv_wykladowca[id].wait(lock_1,[&id](){return vec_active_wykladowca[id] == 1 || endFlag == 1;});

        if (endFlag == 1)
            break;

		//printw("wykladowca %d\n",id);

        int  time = rand()%2000 + 2000;
		std::this_thread::sleep_for(std::chrono::milliseconds(time));

        std::unique_lock<std::mutex>lock(mx_magazyn);
        cv_magazyn.wait(lock,[]{return warehouse > min_warehouse || endFlag == 1;});
        if (endFlag == 1)
            break;
        warehouse--;
        lock.unlock();
        cv_magazyn.notify_all();


		std::unique_lock<std::mutex> lock1(mx_zasoby);
		cv_zasoby.wait(lock1, []{return store < max_store || endFlag == 1;});
		if (endFlag == 1)
			break;
		store++;
		//printw("Producent produkuje %d\tZasoby: %d\n",id,store);

		lock1.unlock();
		cv_zasoby.notify_all();
		time = rand()%1000 + 2000;
		std::this_thread::sleep_for(std::chrono::milliseconds(time));


	}
}

void dostawca(int id)
{
    while (!endFlag)
    {
        std::unique_lock<std::mutex> lock(mx_magazyn);
        cv_magazyn.wait(lock,[]{return warehouse < max_warehouse || endFlag == 1;});
        if (endFlag == 1)
            break;
        int dostawa = rand()%(max_warehouse - warehouse) + 1;
        warehouse += dostawa/2;
        //warehouse+=2;
        lock.unlock();
        cv_magazyn.notify_all();
		int time = rand()%2000 + 2000;
		std::this_thread::sleep_for(std::chrono::milliseconds(time));
    }
}


void control()
{

	//std::this_thread::sleep_for(std::chrono::milliseconds(500));
	while (!endFlag || 1)
	{
		//std::unique_lock<std::mutex> lock(m);
        timeout(20);
		char a;
		a=getch();

		//if ((int)a != -1)printw("koniec control \n");
		//std::this_thread::sleep_for(std::chrono::milliseconds(500));
		//std::cout<<" endFlag: "<<endFlag<<"\n input: "<<a<<"\n";
		if (a == 'q')
		{
			printw("koniec control \n");
			endFlag = 1;

            notify_all_clients();
            notify_all_kasjerzy();
            notify_all_wykladowcy();
            cv_zasoby.notify_all();
		}
		else if (a == 'a')
		{
            if (l_aktywnych_klientow < l_client)
            {
                vec_active_client[l_aktywnych_klientow] = 1;
                vec_cv_client[l_aktywnych_klientow].notify_all();
                l_aktywnych_klientow++;
            }
		}
		else if (a == 'z')
		{
            if (l_aktywnych_klientow > 0)
            {
                --l_aktywnych_klientow;
                vec_active_client[l_aktywnych_klientow] = 0;
            }
		}
		else if (a == 's')
		{
            if (l_aktywnych_kasjerow < l_kasjer)
            {
                vec_active_queue[l_aktywnych_kasjerow] = 1;
                vec_active_kasjer[l_aktywnych_kasjerow] = 1;
                vec_cv_kasjer[l_aktywnych_kasjerow].notify_all();
                l_aktywnych_kasjerow++;
            }
		}
		else if (a == 'x')
		{
            if (l_aktywnych_kasjerow  > 0)
            {
                --l_aktywnych_kasjerow;
                vec_active_queue[l_aktywnych_kasjerow] = 0;
                vec_active_kasjer[l_aktywnych_kasjerow] = 0;
            }
		}
		else if (a == 'd')
		{
            if (l_aktywnych_wykladowcow < l_wykladowca)
            {
                vec_active_wykladowca[l_aktywnych_wykladowcow] = 1;
                vec_cv_wykladowca[l_aktywnych_wykladowcow].notify_all();
                l_aktywnych_wykladowcow++;
            }
		}
		else if (a == 'c')
		{
            if (l_aktywnych_wykladowcow  > 0)
            {
                --l_aktywnych_wykladowcow;
                vec_active_wykladowca[l_aktywnych_wykladowcow] = 0;
            }
		}

		//std::this_thread::sleep_for(std::chrono::milliseconds(500));
	}
	//printw("koniec control \n");
}

void automated_control()
{
    while (!endFlag)
    {
        mvprintw(5,30,"auto");
        if (l_aktywnych_kasjerow == 0 && l_client_shopping != 0)
        {
            //if need deacivate wykladowca
            if (l_aktywnych_kasjerow + l_aktywnych_wykladowcow == l_pracownicy && l_aktywnych_wykladowcow  > 0)
            {
                --l_aktywnych_wykladowcow;
                vec_active_wykladowca[l_aktywnych_wykladowcow] = 0;
            }
            // activate kasjer
            if (l_aktywnych_kasjerow < l_kasjer)
            {
                vec_active_queue[l_aktywnych_kasjerow] = 1;
                vec_active_kasjer[l_aktywnych_kasjerow] = 1;
                vec_cv_kasjer[l_aktywnych_kasjerow].notify_all();
                l_aktywnych_kasjerow++;
            }
        }
        if (store < min_store + 0.6*(max_store-min_store))
        {
            //if need deactivate kasjer
            if ( l_aktywnych_kasjerow + l_aktywnych_wykladowcow == l_pracownicy && l_aktywnych_kasjerow  > 0)
            {
                --l_aktywnych_kasjerow;
                vec_active_queue[l_aktywnych_kasjerow] = 0;
                vec_active_kasjer[l_aktywnych_kasjerow] = 0;
            }
            //activate wykladowca
            if (l_aktywnych_wykladowcow < l_pracownicy)
            {
                vec_active_wykladowca[l_aktywnych_wykladowcow] = 1;
                vec_cv_wykladowca[l_aktywnych_wykladowcow].notify_all();
                l_aktywnych_wykladowcow++;
            }
        }
        else if (l_client_in_queue > 5*l_aktywnych_kasjerow)
        {
            //if need deacivate wykladowca
            if (l_aktywnych_kasjerow + l_aktywnych_wykladowcow == l_pracownicy && l_aktywnych_wykladowcow  > 0)
            {
                --l_aktywnych_wykladowcow;
                vec_active_wykladowca[l_aktywnych_wykladowcow] = 0;
            }
            // activate kasjer
            if (l_aktywnych_kasjerow < l_kasjer)
            {
                vec_active_queue[l_aktywnych_kasjerow] = 1;
                vec_active_kasjer[l_aktywnych_kasjerow] = 1;
                vec_cv_kasjer[l_aktywnych_kasjerow].notify_all();
                l_aktywnych_kasjerow++;
            }
        }
        else if (store >= min_store + 1 *(max_store - min_store) -1)
        {
            // deacivate wykladowca
            if (l_aktywnych_wykladowcow  > 0)
            {
                --l_aktywnych_wykladowcow;
                vec_active_wykladowca[l_aktywnych_wykladowcow] = 0;
            }
        }
        else if (l_client_in_queue < 0.5*l_aktywnych_kasjerow)
        {
            // deactivate kasjer
            if (l_aktywnych_kasjerow  > 0)
            {
                --l_aktywnych_kasjerow;
                vec_active_queue[l_aktywnych_kasjerow] = 0;
                vec_active_kasjer[l_aktywnych_kasjerow] = 0;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(2000));

    }

}

void displayer()
{
    while(!endFlag)
    {
        clear();

        mvprintw(4,5,"Magazyn: %d / %d",warehouse,max_warehouse);
        mvprintw(5,5,"Zasoby: %d / %d",store,max_store);
        mvprintw(6,5,"Aktywni klienci: %d / %d",l_aktywnych_klientow,l_client);
        mvprintw(7,5,"\tklienci w sklepie: %d",l_client_in_store);
        mvprintw(8,5,"\tklienci wsrod polek: %d",l_client_shopping);
        mvprintw(9,5,"\tklienci w kolejkach: %d",l_client_in_queue);
        mvprintw(10,5,"Aktywni wykladowcy: %d / %d",l_aktywnych_wykladowcow,l_pracownicy);
        mvprintw(11,5,"Aktywni kasjerzy: %d / %d",l_aktywnych_kasjerow,l_kasjer);
        mvprintw(12,5,"Nieaktywni pracownicy: %d",l_pracownicy-l_aktywnych_kasjerow-l_aktywnych_wykladowcow);
        mvprintw(15,5,"Stan kolejek:");
        int i;
        for (i=0;i<l_kasjer;++i)
        {
            if (vec_active_queue[i] == 1 || vec_queue[i].size() != 0)
                mvprintw(16+i,5,"\tkolejka %d: %d",i,vec_queue[i].size());
            else
                mvprintw(16+i,5,"\tkolejka %d: nieaktywna",i);
        }

        mvprintw(i+16+5,5,"q - wyjscie");
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

int main(int argc, char** argv) // argv[1] - cons, argv[2] - produc
{
	if (argc != 5)
	{
        std::cout<<"Za malo argumentow [l_klient, l_kasjer, l_wykladowca, l_dostawcy]\n";
        return -1;
	}
	initscr();
	raw();
	srand(time(0));

    l_client = atoi(argv[1]);
    l_kasjer = atoi(argv[2]);
    l_wykladowca = atoi(argv[3]);
    l_dostawcy = atoi(argv[4]);
    l_pracownicy = l_kasjer + l_wykladowca;
    min_store = 0;
    max_store = 20;
    min_warehouse = 0;
    max_warehouse = 50;

    vec_queue = new std::queue<int>[l_kasjer];
    vec_active_queue = new int [l_kasjer];

    vec_mx_client = new std::mutex [l_client];
    vec_cv_client = new std::condition_variable[l_client];
    vec_active_client = new int [l_client];
    vec_client_in_queue = new int [l_client];

    vec_mx_kasjer = new std::mutex [l_kasjer];
    vec_cv_kasjer = new std::condition_variable[l_kasjer];
    vec_active_kasjer = new int [l_kasjer];

    vec_mx_wykladowca = new std::mutex [l_pracownicy];
    vec_cv_wykladowca = new std::condition_variable[l_pracownicy];
    vec_active_wykladowca = new int [l_pracownicy];

    for (int i=0;i<l_client;++i)
        vec_active_client[i] = 0;
    for (int i=0;i<l_kasjer;++i)
        vec_active_kasjer[i] = 0;
    for (int i=0;i<l_pracownicy;++i)
        vec_active_wykladowca[i] = 0;

    std::thread thread_display;
	std::thread execution_control;
    std::thread thread_automated_control;
    std::thread *vec_thread_dostawca = new std::thread[l_dostawcy];
	std::thread *vec_thread_client = new std::thread[l_client];
	std::thread *vec_thread_kasjer = new std::thread[l_kasjer];
	std::thread *vec_thread_wykladowca = new std::thread[l_pracownicy];

    thread_display = std::thread(displayer);
	execution_control = std::thread(control);
	thread_automated_control = std::thread(automated_control);

	for (int i=0;i<l_client;++i)
		vec_thread_client[i] = std::thread(consumer,i);
	for (int i=0;i<l_kasjer;++i)
		vec_thread_kasjer[i] = std::thread(kasjer,i);
	for (int i=0;i<l_pracownicy;++i)
		vec_thread_wykladowca[i] = std::thread(wykladowca,i);
    for (int i=0;i<l_dostawcy;++i)
        vec_thread_dostawca[i] = std::thread(dostawca,i);


	for (int i=0;i<l_client;++i)
		vec_thread_client[i].join();
    printw("lol1\n");
    for (int i=0;i<l_kasjer;++i)
		vec_thread_kasjer[i].join();
    printw("lol2\n");
    for (int i=0;i<l_pracownicy;++i)
		vec_thread_wykladowca[i].join();
    for (int i=0;i<l_dostawcy;++i)
        vec_thread_dostawca[i].join();

    printw("lol3\n");
	execution_control.join();
	printw("lol4\n");
	thread_automated_control.join();
	printw("lol5\n");
	//endFlagDisplay = 1;
	thread_display.join();
	printw("lol6\n");
	endwin();

	return 0;

}


void notify_all_clients()
{
    for (int i=0;i<l_client;++i)
        vec_cv_client[i].notify_all();

}
void notify_all_kasjerzy()
{
    for (int i=0;i<l_client;++i)
        vec_cv_kasjer[i].notify_all();

}
void notify_all_wykladowcy()
{
    for (int i=0;i<l_client;++i)
        vec_cv_wykladowca[i].notify_all();
}
