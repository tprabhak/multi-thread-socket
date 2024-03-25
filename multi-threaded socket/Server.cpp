#include "thread.h"
#include "socketserver.h"
#include <stdlib.h>
#include <time.h>
#include <list>
#include <vector>
#include <algorithm>

using namespace Sync;

// SOCKET
class SocketThread : public Thread{
	private:
		Socket& socket;
		bool &terminate;
        ByteArray name;
		ByteArray data;
		std::vector<SocketThread*> &sockThrHolder;
	public:
		// SOCKET Consturctor
		SocketThread(Socket& socket, bool& terminate, std::vector<SocketThread*> &clientSockThr): socket(socket), terminate(terminate), sockThrHolder(clientSockThr){}
		// Destructor
		~SocketThread(){
			this->terminationEvent.Wait();
		}

		Socket& GetSocket(){
		    return socket;
		}

    virtual long ThreadMain(){
        //Create semaphore on each socket thread by userName
        //Semaphore clientBlocker(userName);
        // First read the client username
        socket.Read(name);
        std::string n = name.ToString();
        std::cout << n << " has joined" << std::endl;
        for(const auto& thread: sockThrHolder) {
            // Broadcast the user JOIN
            if(thread != this) {
                thread->GetSocket().Write(ByteArray(n + " has joined"));
            }
        }

        try{
            while(!terminate){
                socket.Read(data);
                std::string res = data.ToString();

                if (res == "done") {
                    std::cout << n << " has left the chatroom" << std::endl;
                    for(const auto& thread: sockThrHolder) {
                        // Broadcast the user LEFT
                        if(thread != this) {
                            thread->GetSocket().Write(ByteArray(n + " has left the chatroom"));
                        }
                    }
                    // terminate = true;
                    break;  
                }

                std::cout << n << ": " << res << std::endl;

                for(const auto& thread: sockThrHolder) {
                    // Broadcast the user input MSG
                    if(thread != this) {
                        thread->GetSocket().Write(ByteArray(n + ": " + res));
                    }
                }
            } 
        }
        catch (std::string &s) {
            std::cout<<s<<std::endl;
        }
        
        catch (std::string err){
            std::cout<<err<<std::endl;
        }
        
        std::cout<<"Client Disconnected" <<std::endl;
        
        return 0;
    }

};

// SERVER
class ServerThread : public Thread
{
private:
    SocketServer& server;
    bool terminate = false;
    std::vector<SocketThread*> sockThrHolder;
public:
	// Constructor
    ServerThread(SocketServer& server): server(server){}
	// Destructor
    ~ServerThread(){
        // Cleanup
        for(const auto& thread: sockThrHolder) {
            try{
                Socket& toClose = thread->GetSocket();
                toClose.Write(ByteArray("server_termination"));
                toClose.Close();
                // thread->terminate = true;
                // thread->GetSocket().Close();
                // thread->GetSocket().Write(ByteArray("server_termination"));

            } catch (...) {

            }
        } 
        // std::vector<SocketThread*>().swap(sockThrHolder);
        std::cout<<"Closing client from server"<<std::endl;
        terminate = true;
        
    }

    virtual long ThreadMain(){
        
        // welcome msg
        std::cout<<"\n\t  ====== Welcome to the chatGPThi ======   "<< std::endl;

        while (1) {
            try {
                // Wait for a client socket 
                Socket* newConnection = new Socket(server.Accept());

                ThreadSem serverBlock(1);

                // Pass a reference to this pointer 
                Socket& socketReference = *newConnection;
                sockThrHolder.push_back(new SocketThread(socketReference, terminate, std::ref(sockThrHolder)));
            } catch (std::string error)
            {
                std::cout << "ERROR: " << error << std::endl;
				
                return 1;
            }
			
			catch (TerminationException terminationException)
			{
				std::cout << "Server has shut down!" << std::endl;
				
				return terminationException;
			}
        }
        return 0;
    }
};

int main(void)
{
    std::cout << "I am a server." << std::endl;
    std::cout << "Press 'enter' key to terminate the server...";
    std::cout.flush();
    
    // Create server
    SocketServer server(3000); 
    
    // Need a thread to perform server operations
    ServerThread serverThread(server);
	
    // This will wait for input to shutdown the server
    FlexWait cinWaiter(1, stdin);
    cinWaiter.Wait();
    std::cin.get();

    // // Shut down
    server.Shutdown();
}
