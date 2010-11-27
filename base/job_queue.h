namespace htools {

class JobQueue {
  struct         Private;
  Private* const _d;
public:
  JobQueue(const char* name);
  ~JobQueue();
  void open(size_t max_size = 1);
  void close();
  void wait();
  bool empty() const;
  size_t size() const;
  int push(void* data);
  int pop(void** data);
};

};
