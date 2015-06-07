#include <iostream>
#include <fstream>
#include <iomanip>
#include <string>
#include <vector>
#include <sstream> 
#include <set>
#include <map>
#include <conio.h>
#include <windows.h>


const int default_name_length = 150;
	
#define EVENT_COUNT 2

#define FIRST 1
#define SECOND 2
#define THIRD 3

class Student{
private:
	int num;
	char name[default_name_length];
	double grade;
public:
	Student(int _num, std::string _name, double _grade) :num(_num), grade(_grade){
		std::copy(_name.begin(), _name.end(), name);
		name[_name.size()] = '\0';
	}
	Student() :num(0), grade(0.0){
		name[0] = '\0';
	}
	int get_id(){
		return this->num;
	}
	std::string get_name(){
		return this->name;
	}
	double get_grade(){
		return this->grade;
	}
	void set_name(std::string _name){
		std::copy(_name.begin(), _name.end(), name);
		name[_name.size()] = '\0';
	}
	void  set_grade(double grade){
		this->grade = grade;
	}
	std::string stud_to_string(){
		return std::to_string(num) + "," + name + "," + std::to_string(grade);
	}
};


class Client{
private:
	HANDLE hWritePipe, hReadPipe;
	HANDLE hMutex;
	char* lpszMutex = "MutexName";
	HANDLE hReadModifyRequestEvents[3];
	HANDLE hReadModifyResponseEvents[3];
	char* lpszEnableReadRequest = "EnableReadRequset";
	char* lpszEnableModifyRequest = "EnableModifyRequest";
	char* lpszEnableReadResponse = "EnableReadResponse";
	char* lpszEnableModifyResponse = "EnableModifyResponse";
	char* lpszCheckAccessRequest = "CheckAccessRequest";
	char* lpszCheckAccessResponse = "CheckAccessResponse";
public:
	Client(char* argv[]){
		hWritePipe = (HANDLE)atoi(argv[1]);
		hReadPipe = (HANDLE)atoi(argv[2]);

		hReadModifyRequestEvents[0] = OpenEvent(EVENT_ALL_ACCESS, FALSE, lpszEnableReadRequest);
		hReadModifyRequestEvents[1] = OpenEvent(EVENT_ALL_ACCESS, FALSE, lpszEnableModifyRequest);
		hReadModifyRequestEvents[2] = OpenEvent(EVENT_ALL_ACCESS, FALSE, lpszCheckAccessRequest);

		hReadModifyResponseEvents[0] = OpenEvent(EVENT_ALL_ACCESS, FALSE, lpszEnableReadResponse);
		hReadModifyResponseEvents[1] = OpenEvent(EVENT_ALL_ACCESS, FALSE, lpszEnableModifyResponse);
		hReadModifyResponseEvents[2] = OpenEvent(EVENT_ALL_ACCESS, FALSE, lpszCheckAccessResponse);

		hMutex = OpenMutex(SYNCHRONIZE, FALSE, lpszMutex);
	}
	void loop_of_choice(){
		char choice;
		int id;
		std::string new_name;
		double new_grade;
		Student new_stud;
		DWORD dwBytesWritten;
		DWORD dwBytesRead;
		std::pair<const char*, double> tmp;
		do{
			std::cout << "Make your choice\n" <<
				"1. Read\n" <<
				"2. Modify\n" <<
				"3. Exit\n";
			choice = _getch();
			switch (choice){
			case '1':{
				Student temp;
				std::cout << "Enter ID to read : ";
				std::cin >> id;
				std::cout << std::endl;
				WriteFile(hWritePipe, &id, sizeof(id), &dwBytesWritten, NULL);
				SetEvent(hReadModifyRequestEvents[2]);
				WaitForSingleObject(hReadModifyResponseEvents[2], INFINITE);
				int response = 0;
				ReadFile(hReadPipe, &response, sizeof(unsigned int), &dwBytesRead, NULL);
				if (response){
					WriteFile(hWritePipe, &id, sizeof(id), &dwBytesWritten, NULL);
					SetEvent(hReadModifyRequestEvents[0]);
					WaitForSingleObject(hReadModifyResponseEvents[0], INFINITE);
					ReadFile(hReadPipe, &temp, sizeof(temp), &dwBytesRead, NULL);
					if (temp.get_id()){
						std::cout << temp.stud_to_string() << std::endl;
					}
					else{
						std::cout << "No such student!\n";
					}
				}
				else{
					std::cout << "Access denied!\n";
				}
				
				break;
			}
			case '2':{
				Student temp;
				std::cout << "Enter ID to modify : ";
				std::cin >> id;
				std::cout << std::endl;
				WriteFile(hWritePipe, &id, sizeof(id), &dwBytesWritten, NULL);
				SetEvent(hReadModifyRequestEvents[2]);
				WaitForSingleObject(hReadModifyResponseEvents[2], INFINITE);
				int response = 0;
				ReadFile(hReadPipe, &response, sizeof(unsigned int), &dwBytesRead, NULL);
				if (response != 2){
					if (response){
						std::cout << "Enter new data :\n" <<
							"1. New name : ";
						std::cin >> new_name;
						std::cout << "\n2. New grade : ";
						std::cin >> new_grade;
						std::cout << std::endl;
						new_stud = Student(id, new_name, new_grade);
						WriteFile(hWritePipe, &new_stud, sizeof(new_stud), &dwBytesWritten, NULL);
						SetEvent(hReadModifyRequestEvents[1]);
						WaitForSingleObject(hReadModifyResponseEvents[1], INFINITE);
					}
					else{
						std::cout << "Access denied!\n";
					}
				}
				else{
					std::cout << "No such student!\n";
				}
				break;
			}
			case '3':
				return;
			default: 
				std::cout << "You've entered wrong number! Try again or exit!" << std::endl;
			}
		} while (true);
	}
};


int main(int argc, char* argv[]){

	Client client(argv);
	client.loop_of_choice();

	return 0;
}