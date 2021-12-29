#include <javm/javm_VM.hpp>

// TODO: order classes in better way

#include <javm/native/impl/java/lang/lang_Object.hpp>
#include <javm/native/impl/java/lang/lang_System.hpp>
#include <javm/native/impl/java/lang/lang_Class.hpp>
#include <javm/native/impl/java/lang/lang_ClassLoader.hpp>
#include <javm/native/impl/java/security/security_AccessController.hpp>
#include <javm/native/impl/java/lang/lang_Float.hpp>
#include <javm/native/impl/java/lang/lang_Double.hpp>
#include <javm/native/impl/sun/misc/misc_VM.hpp>
#include <javm/native/impl/sun/reflect/reflect_Reflection.hpp>
#include <javm/native/impl/sun/misc/misc_Unsafe.hpp>
#include <javm/native/impl/java/lang/lang_Throwable.hpp>
#include <javm/native/impl/java/lang/lang_String.hpp>
#include <javm/native/impl/java/io/io_FileDescriptor.hpp>
#include <javm/native/impl/java/lang/lang_Thread.hpp>
#include <javm/native/impl/java/io/io_FileInputStream.hpp>
#include <javm/native/impl/java/io/io_FileOutputStream.hpp>
#include <javm/native/impl/sun/nio/cs/cs_StreamEncoder.hpp>
#include <javm/native/impl/java/io/io_WinNTFileSystem.hpp>
#include <javm/native/impl/java/util/concurrent/atomic/atomic_AtomicLong.hpp>
#include <javm/native/impl/sun/misc/misc_Signal.hpp>
#include <javm/native/impl/sun/io/io_Win32ErrorMode.hpp>

namespace javm::native {

    void RegisterNativeStandardImplementation() {
        RegisterNativeClassMethod(u"java/lang/Object", u"registerNatives", u"()V", &impl::java::lang::Object::registerNatives);
        RegisterNativeInstanceMethod(u"java/lang/Object", u"getClass", u"()Ljava/lang/Class;", &impl::java::lang::Object::getClass);
        RegisterNativeInstanceMethod(u"java/lang/Object", u"hashCode", u"()I", &impl::java::lang::Object::hashCode);
        RegisterNativeInstanceMethod(u"java/lang/Object", u"notify", u"()V", &impl::java::lang::Object::notify);
        RegisterNativeInstanceMethod(u"java/lang/Object", u"notifyAll", u"()V", &impl::java::lang::Object::notifyAll);
        RegisterNativeInstanceMethod(u"java/lang/Object", u"wait", u"(J)V", &impl::java::lang::Object::wait);
        RegisterNativeClassMethod(u"java/lang/System", u"registerNatives", u"()V", &impl::java::lang::System::registerNatives);
        RegisterNativeClassMethod(u"java/lang/System", u"initProperties", u"(Ljava/util/Properties;)Ljava/util/Properties;", &impl::java::lang::System::initProperties);
        RegisterNativeClassMethod(u"java/lang/System", u"arraycopy", u"(Ljava/lang/Object;ILjava/lang/Object;II)V", &impl::java::lang::System::arraycopy);
        RegisterNativeClassMethod(u"java/lang/System", u"setIn0", u"(Ljava/io/InputStream;)V", &impl::java::lang::System::setIn0);
        RegisterNativeClassMethod(u"java/lang/System", u"setOut0", u"(Ljava/io/PrintStream;)V", &impl::java::lang::System::setOut0);
        RegisterNativeClassMethod(u"java/lang/System", u"setErr0", u"(Ljava/io/PrintStream;)V", &impl::java::lang::System::setErr0);
        RegisterNativeClassMethod(u"java/lang/System", u"mapLibraryName", u"(Ljava/lang/String;)Ljava/lang/String;", &impl::java::lang::System::mapLibraryName);
        RegisterNativeClassMethod(u"java/lang/System", u"loadLibrary", u"(Ljava/lang/String;)V", &impl::java::lang::System::loadLibrary);
        RegisterNativeClassMethod(u"java/lang/System", u"currentTimeMillis", u"()J", &impl::java::lang::System::currentTimeMillis);
        RegisterNativeClassMethod(u"java/lang/System", u"identityHashCode", u"(Ljava/lang/Object;)I", &impl::java::lang::System::identityHashCode);
        RegisterNativeClassMethod(u"java/lang/Class", u"registerNatives", u"()V", &impl::java::lang::Class::registerNatives);
        RegisterNativeClassMethod(u"java/lang/Class", u"getPrimitiveClass", u"(Ljava/lang/String;)Ljava/lang/Class;", &impl::java::lang::Class::getPrimitiveClass);
        RegisterNativeClassMethod(u"java/lang/Class", u"desiredAssertionStatus0", u"(Ljava/lang/Class;)Z", &impl::java::lang::Class::desiredAssertionStatus0);
        RegisterNativeClassMethod(u"java/lang/Class", u"forName0", u"(Ljava/lang/String;ZLjava/lang/ClassLoader;Ljava/lang/Class;)Ljava/lang/Class;", &impl::java::lang::Class::forName0);
        RegisterNativeInstanceMethod(u"java/lang/Class", u"getDeclaredFields0", u"(Z)[Ljava/lang/reflect/Field;", &impl::java::lang::Class::getDeclaredFields0);
        RegisterNativeInstanceMethod(u"java/lang/Class", u"isInterface", u"()Z", &impl::java::lang::Class::isInterface);
        RegisterNativeInstanceMethod(u"java/lang/Class", u"isPrimitive", u"()Z", &impl::java::lang::Class::isPrimitive);
        RegisterNativeInstanceMethod(u"java/lang/Class", u"isAssignableFrom", u"(Ljava/lang/Class;)Z", &impl::java::lang::Class::isAssignableFrom);
        RegisterNativeClassMethod(u"java/lang/ClassLoader", u"registerNatives", u"()V", &impl::java::lang::ClassLoader::registerNatives);
        RegisterNativeClassMethod(u"java/security/AccessController", u"doPrivileged", u"(Ljava/security/PrivilegedExceptionAction;)Ljava/lang/Object;", &impl::java::security::AccessController::doPrivileged);
        RegisterNativeClassMethod(u"java/security/AccessController", u"doPrivileged", u"(Ljava/security/PrivilegedAction;)Ljava/lang/Object;", &impl::java::security::AccessController::doPrivileged);
        RegisterNativeClassMethod(u"java/security/AccessController", u"getStackAccessControlContext", u"()Ljava/security/AccessControlContext;", &impl::java::security::AccessController::getStackAccessControlContext);
        RegisterNativeClassMethod(u"java/lang/Float", u"floatToRawIntBits", u"(F)I", &impl::java::lang::Float::floatToRawIntBits);
        RegisterNativeClassMethod(u"java/lang/Double", u"doubleToRawLongBits", u"(D)J", &impl::java::lang::Double::doubleToRawLongBits);
        RegisterNativeClassMethod(u"java/lang/Double", u"longBitsToDouble", u"(J)D", &impl::java::lang::Double::longBitsToDouble);
        RegisterNativeClassMethod(u"sun/misc/VM", u"initialize", u"()V", &impl::sun::misc::VM::initialize);
        RegisterNativeClassMethod(u"sun/reflect/Reflection", u"getCallerClass", u"()Ljava/lang/Class;", &impl::sun::reflect::Reflection::getCallerClass);
        RegisterNativeClassMethod(u"sun/reflect/Reflection", u"getClassAccessFlags", u"(Ljava/lang/Class;)I", &impl::sun::reflect::Reflection::getClassAccessFlags);
        RegisterNativeClassMethod(u"sun/misc/Unsafe", u"registerNatives", u"()V", &impl::sun::misc::Unsafe::registerNatives);
        RegisterNativeInstanceMethod(u"sun/misc/Unsafe", u"arrayBaseOffset", u"(Ljava/lang/Class;)I", &impl::sun::misc::Unsafe::arrayBaseOffset);
        RegisterNativeInstanceMethod(u"sun/misc/Unsafe", u"arrayIndexScale", u"(Ljava/lang/Class;)I", &impl::sun::misc::Unsafe::arrayIndexScale);
        RegisterNativeInstanceMethod(u"sun/misc/Unsafe", u"addressSize", u"()I", &impl::sun::misc::Unsafe::addressSize);
        RegisterNativeInstanceMethod(u"sun/misc/Unsafe", u"objectFieldOffset", u"(Ljava/lang/reflect/Field;)J", &impl::sun::misc::Unsafe::objectFieldOffset);
        RegisterNativeInstanceMethod(u"sun/misc/Unsafe", u"getIntVolatile", u"(Ljava/lang/Object;J)I", &impl::sun::misc::Unsafe::getIntVolatile);
        RegisterNativeInstanceMethod(u"sun/misc/Unsafe", u"compareAndSwapInt", u"(Ljava/lang/Object;JII)Z", &impl::sun::misc::Unsafe::compareAndSwapInt);
        RegisterNativeInstanceMethod(u"sun/misc/Unsafe", u"allocateMemory", u"(J)J", &impl::sun::misc::Unsafe::allocateMemory);
        RegisterNativeInstanceMethod(u"sun/misc/Unsafe", u"putLong", u"(JJ)V", &impl::sun::misc::Unsafe::putLong);
        RegisterNativeInstanceMethod(u"sun/misc/Unsafe", u"getByte", u"(J)B", &impl::sun::misc::Unsafe::getByte);
        RegisterNativeInstanceMethod(u"sun/misc/Unsafe", u"freeMemory", u"(J)V", &impl::sun::misc::Unsafe::freeMemory);
        RegisterNativeInstanceMethod(u"java/lang/Throwable", u"fillInStackTrace", u"(I)Ljava/lang/Throwable;", &impl::java::lang::Throwable::fillInStackTrace);
        RegisterNativeInstanceMethod(u"java/lang/Throwable", u"getStackTraceDepth", u"()I", &impl::java::lang::Throwable::getStackTraceDepth);
        RegisterNativeInstanceMethod(u"java/lang/Throwable", u"getStackTraceElement", u"(I)Ljava/lang/StackTraceElement;", &impl::java::lang::Throwable::getStackTraceElement);
        RegisterNativeInstanceMethod(u"java/lang/String", u"intern", u"()Ljava/lang/String;", &impl::java::lang::String::intern);
        RegisterNativeClassMethod(u"java/io/FileDescriptor", u"initIDs", u"()V", &impl::java::io::FileDescriptor::initIDs);
        RegisterNativeClassMethod(u"java/io/FileDescriptor", u"set", u"(I)J", &impl::java::io::FileDescriptor::set);
        RegisterNativeClassMethod(u"java/lang/Thread", u"registerNatives", u"()V", &impl::java::lang::Thread::registerNatives);
        RegisterNativeClassMethod(u"java/lang/Thread", u"currentThread", u"()Ljava/lang/Thread;", &impl::java::lang::Thread::currentThread);
        RegisterNativeInstanceMethod(u"java/lang/Thread", u"setPriority0", u"(I)V", &impl::java::lang::Thread::setPriority0);
        RegisterNativeInstanceMethod(u"java/lang/Thread", u"isAlive", u"()Z", &impl::java::lang::Thread::isAlive);
        RegisterNativeInstanceMethod(u"java/lang/Thread", u"start0", u"()V", &impl::java::lang::Thread::start0);
        RegisterNativeClassMethod(u"java/io/FileInputStream", u"initIDs", u"()V", &impl::java::io::FileInputStream::initIDs);
        RegisterNativeClassMethod(u"java/io/FileOutputStream", u"initIDs", u"()V", &impl::java::io::FileOutputStream::initIDs);
        RegisterNativeInstanceMethod(u"java/io/FileOutputStream", u"writeBytes", u"([BIIZ)V", &impl::java::io::FileOutputStream::writeBytes);
        RegisterNativeClassMethod(u"sun/nio/cs/StreamEncoder", u"forOutputStreamWriter", u"(Ljava/io/OutputStream;Ljava/lang/Object;Ljava/lang/String;)Lsun/nio/cs/StreamEncoder;", &impl::sun::nio::cs::StreamEncoder::forOutputStreamWriter);
        RegisterNativeClassMethod(u"java/io/WinNTFileSystem", u"initIDs", u"()V", &impl::java::io::WinNTFileSystem::initIDs);
        RegisterNativeClassMethod(u"java/util/concurrent/atomic/AtomicLong", u"VMSupportsCS8", u"()Z", &impl::java::util::concurrent::atomic::AtomicLong::VMSupportsCS8);
        RegisterNativeClassMethod(u"sun/misc/Signal", u"findSignal", u"(Ljava/lang/String;)I", &impl::sun::misc::Signal::findSignal);
        RegisterNativeClassMethod(u"sun/misc/Signal", u"handle0", u"(IJ)J", &impl::sun::misc::Signal::handle0);
        RegisterNativeClassMethod(u"sun/io/Win32ErrorMode", u"setErrorMode", u"(J)J", &impl::sun::io::Win32ErrorMode::setErrorMode);
    }

}