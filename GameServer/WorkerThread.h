#pragma once

class WorkerThread
{
public:
	WorkerThread() = default;
	~WorkerThread() = default;

	void Disconnect(int clientId);
	void DoWork(HANDLE hIocp);
};
