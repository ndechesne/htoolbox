namespace htools {

class WorkScheduler {
  struct         Private;
  Private* const _d;
  WorkScheduler(const htools::WorkScheduler&);
public:
  typedef void* (*routine_f)(void* data, void* user);
  WorkScheduler(JobQueue& in, JobQueue& out, routine_f routine, void* user);
  ~WorkScheduler();
  int start(size_t max_threads = 0, size_t min_threads = 0);
  int stop();
  size_t threads() const;
};

};
