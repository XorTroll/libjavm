package javm.libnx;

public class Main {

    public static void doThread() {
        Thread thr = Thread.currentThread();
        String thr_name = thr.getName();
        System.out.println("[" + thr_name + "] Priority: " + thr.getPriority());
        for(int i = 0; i < 20; i++) {
            System.out.println("[" + thr_name + "] " + i);
        }
    }

    public static void main(String[] args) {

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