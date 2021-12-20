package javm.test;

public class ClassCastTest {
    public static void main(String[] args) {
        String str = "Gay baby jail";
        try {
            Object o1 = (Object)str;
            System.out.println("Casted String to Object");
        }
        catch(ClassCastException ex) {
            System.out.println("Couldn't cast String to Object");
        }
        try {
            Object o2 = (Object)str;
            Integer i2 = (Integer)o2;
            System.out.println("Casted String to Object to Integer");
        }
        catch(ClassCastException ex) {
            System.out.println("Couldn't cast String to Object to Integer");
        }

        RuntimeException rex = new RuntimeException("Ants");
        try {
            Object o3 = (Object)rex;
            Exception e3 = (Exception)o3;
            System.out.println("Casted RuntimeException to Object to Exception");
        }
        catch(ClassCastException ex) {
            System.out.println("Couldn't cast RuntimeException to Object to Exception");
        }
        try {
            Object o4 = (Object)rex;
            RuntimeException r4 = (RuntimeException)o4;
            System.out.println("Casted RuntimeException to Object to RuntimeException");
        }
        catch(ClassCastException ex) {
            System.out.println("Couldn't cast RuntimeException to Object to RuntimeException");
        }
        try {
            Object o5 = (Object)rex;
            IllegalStateException e5 = (IllegalStateException)o5;
            System.out.println("Casted RuntimeException to Object to IllegalStateException");
        }
        catch(ClassCastException ex) {
            System.out.println("Couldn't cast RuntimeException to Object to IllegalStateException");
        }
        try {
            Object o6 = (Object)rex;
            Throwable e6 = (Throwable)o6;
            RuntimeException r6 = (RuntimeException)e6;
            System.out.println("Casted RuntimeException to Object to Throwable to RuntimeException");
        }
        catch(ClassCastException ex) {
            System.out.println("Couldn't cast RuntimeException to Object to Throwable to RuntimeException");
        }

        System.out.println("Done!");
    }
}
