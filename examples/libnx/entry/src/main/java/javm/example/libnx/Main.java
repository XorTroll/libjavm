package javm.example.libnx;

public class Main {
    public static void doThread() {
        Thread thr = Thread.currentThread();
        String thr_name = thr.getName();
        System.out.println("[" + thr_name + "] Priority: " + thr.getPriority());
        for(int i = 0; i < 10; i++) {
            System.out.println("[" + thr_name + "] " + i);
        }
    }

    public static void main(String[] args) {
        System.out.println("Hello world from Java!");
        System.out.println("Current OS name:    " + System.getProperty("os.name"));
        System.out.println("Current OS arch:    " + System.getProperty("os.arch"));
        System.out.println("Current OS version: " + System.getProperty("os.version"));

        Thread t = new Thread() {
            public void run() {
                doThread();
            }
        };
        t.start();

        doThread();

        try {
            t.join();
            System.out.println("Joined thread!");
        }
        catch(Exception e) {
            System.out.println(e.getClass().getName() + " - " + e.getMessage());
        }
    }
}