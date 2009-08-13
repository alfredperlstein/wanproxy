#ifndef	SOCKET_H
#define	SOCKET_H

enum SocketAddressFamily {
	SocketAddressFamilyIPv4,
	SocketAddressFamilyIPv6,
	SocketAddressFamilyUnix,
};

enum SocketType {
	SocketTypeStream,
	SocketTypeDatagram,
};

class Socket : public FileDescriptor {
	LogHandle log_;
	int domain_;
	Action *accept_action_;
	EventCallback *accept_callback_;
	EventCallback *connect_callback_;
	Action *connect_action_;

	Socket(int, int);
public:
	~Socket();

	virtual Action *accept(EventCallback *);
	bool bind(const std::string&);
	Action *connect(const std::string&, EventCallback *);
	bool listen(int=10);

	std::string getpeername(void) const;
	std::string getsockname(void) const;

private:
	void accept_callback(Event);
	void accept_cancel(void);
	Action *accept_schedule(void);

	void connect_callback(Event);
	void connect_cancel(void);
	Action *connect_schedule(void);

public:
	static Socket *create(SocketAddressFamily, SocketType, const std::string&);
};

#endif /* !SOCKET_H */
