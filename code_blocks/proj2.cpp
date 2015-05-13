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

int max_store = 10;
int min_store = 10;

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

int l_pracownicy;

//liczba kas = liczba kasjerow
std::mutex mx_queue; // watek kierownika sklepu musi byc w stanie, klient musi miec czas by sprawdzic ktora kolejka jest najkrotsza
int* vec_active_queue;
int* vec_client_in_queue;
std::queue<int> *vec_queue;

int endFlag;







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

		printw("klient %d\n",id);

		l_client_in_store++;
		int  time = rand()%500 + 1000;


		std::unique_lock<std::mutex> lock_2(mx_zasoby);
		cv_zasoby.wait(lock_2, []{return store >= min_store || endFlag == 1;});
		if (endFlag == 1)
			break;
		store--;
		printw("klient asdasd %d\n",id);
		std::this_thread::sleep_for(std::chrono::milliseconds(time));
		printw("Klient bierze towar %d\tZasoby: %d\n",id,store);

		lock_2.unlock();



		int min_queue_nr=-1;
		do{
            if (endFlag == 1)
                return;
            std::unique_lock<std::mutex> lock(mx_queue);
            for (int i=0 ;i < l_kasjer; ++i)
                if (vec_active_queue[i] == 1)
                    min_queue_nr = i;
            for (int i=0 ;i < l_kasjer; ++i)
                if (vec_active_queue[i] == 1 && vec_queue[i].size()<vec_queue[min_queue_nr].size())
                    min_queue_nr = i;
            lock.unlock();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        while (min_queue_nr == -1);
		printw("klient %d wybral kolejke %d\n",id,min_queue_nr);

		vec_queue[min_queue_nr].push(id);
		vec_client_in_queue[id] = 1;
		//stanie w kolejce
		l_client_in_queue++;
		std::unique_lock<std::mutex> lock_waiting_in_queue(vec_mx_client[id]);
        vec_cv_client[id].wait(lock_waiting_in_queue,[&id](){return vec_client_in_queue[id] == 0 || endFlag == 1;});
        l_client_in_queue--;
		//wychodzi ze sklepu
		printw("klient %d wychodzi z sklepu\n");

		time = rand()%500 + 1000;
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

		printw("kasjer %d\n",id);
        int  time = rand()%500 + 1000;
		std::this_thread::sleep_for(std::chrono::milliseconds(time));

		while (vec_queue[id].empty() == false)
		{
            std::unique_lock<std::mutex> lock_2(mx_queue);
            int klient_id = vec_queue[id].front();
            vec_queue[id].pop();
            lock_2.unlock();
            int  time = rand()%500 + 1000;
            std::this_thread::sleep_for(std::chrono::milliseconds(time));
            vec_client_in_queue[klient_id] = 0;

            printw("\tkasjer %d obsluzyl klienta %d\n",id, klient_id);
            vec_cv_kasjer[klient_id].notify_all();

		}
	}
}

void wykladowca(int id)
{


	while (1)
	{

		std::unique_lock<std::mutex> lock_1(vec_mx_wykladowca[id]);
        vec_cv_wykladowca[id].wait(lock_1,[&id](){return vec_active_wykladowca[id] == 1 || endFlag == 1;});

        if (endFlag == 1)
            break;

		printw("wykladowca %d\n",id);

        int  time = rand()%500 + 1000;
		std::this_thread::sleep_for(std::chrono::milliseconds(time));

		std::unique_lock<std::mutex> lock(mx_zasoby);
		cv_zasoby.wait(lock, []{return store < max_store || endFlag == 1;});
		if (endFlag == 1)
			break;
		store++;
		printw("Producent produkuje %d\tZasoby: %d\n",id,store);

		lock.unlock();
		cv_zasoby.notify_all();
		time = rand()%1000 + 2000;
		std::this_thread::sleep_for(std::chrono::milliseconds(time));


	}
}


void control()
{
    int l_aktywnych_klientow=0;
    int l_aktywnych_kasjerow=0;
    int l_aktywnych_wykladowcow=0;
	//std::this_thread::sleep_for(std::chrono::milliseconds(500));
	while (!endFlag)
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
			//printw("koniec control \n");
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

int main(int argc, char** argv) // argv[1] - cons, argv[2] - produc
{


	//int threads_cons = atoi(argv[1]);
	//int threads_prod = atoi(argv[2]);
	if (argc != 4)
	{
        std::cout<<"Za malo argumentow [l_klient, l_kasjer, l_wykladowca]\n";
        return -1;
	}
	initscr();
	raw();
	srand(time(0));

    l_client = atoi(argv[1]);
    l_kasjer = atoi(argv[2]);
    l_wykladowca = atoi(argv[3]);
    l_pracownicy = l_kasjer + l_wykladowca;
    min_store = 0;
    max_store = 10;

    vec_queue = new std::queue<int>[l_kasjer];
    vec_active_queue = new int [l_kasjer];

    vec_mx_client = new std::mutex [l_client];
    vec_cv_client = new std::condition_variable[l_client];
    vec_active_client = new int [l_client];

    vec_mx_kasjer = new std::mutex [l_kasjer];
    vec_cv_kasjer = new std::condition_variable[l_kasjer];
    vec_active_kasjer = new int [l_kasjer];

    vec_mx_wykladowca = new std::mutex [l_wykladowca];
    vec_cv_wykladowca = new std::condition_variable[l_wykladowca];
    vec_active_wykladowca = new int [l_wykladowca];

    for (int i=0;i<l_client;++i)
        vec_active_client[i] = 0;
    for (int i=0;i<l_kasjer;++i)
        vec_active_kasjer[i] = 0;
    for (int i=0;i<l_wykladowca;++i)
        vec_active_wykladowca[i] = 0;



	std::thread execution_control;
	std::thread *vec_thread_client = new std::thread[l_client];
	std::thread *vec_thread_kasjer = new std::thread[l_kasjer];
	std::thread *vec_thread_wykladowca = new std::thread[l_wykladowca];

	execution_control = std::thread(control);

	for (int i=0;i<l_client;++i)
		vec_thread_client[i] = std::thread(consumer,i);
	for (int i=0;i<l_kasjer;++i)
		vec_thread_kasjer[i] = std::thread(kasjer,i);
	for (int i=0;i<l_wykladowca;++i)
		vec_thread_wykladowca[i] = std::thread(wykladowca,i);

	for (int i=0;i<l_client;++i)
		vec_thread_client[i].join();
    for (int i=0;i<l_kasjer;++i)
		vec_thread_kasjer[i].join();
		for (int i=0;i<l_wykladowca;++i)
		vec_thread_wykladowca[i].join();
	//for (int i=0;i<threads_prod;++i)
	//	tab_prod[i].join();
	execution_control.join();
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
