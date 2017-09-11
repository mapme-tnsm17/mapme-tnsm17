# A task is a runnable piece of code
# It implements a run() method
class Task(object):
    # Workaround for Python lack of virtual methods (duck typing is not
    # sufficient for function pointers)
    def virtual_run(self):
        self.run()
