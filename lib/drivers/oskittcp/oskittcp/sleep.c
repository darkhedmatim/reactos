#include <oskittcp.h>
#include <sys/callout.h>
#include <oskitfreebsd.h>
#include <oskitdebug.h>

/* clock_init */
int ncallout = 256;
struct callout *callout;

void init_freebsd_sched() {
}

int tsleep( void *token, int priority, char *wmesg, int tmio ) {
    if( !OtcpEvent.Sleep ) panic("no sleep");
    return
	OtcpEvent.Sleep( OtcpEvent.ClientData, token, priority, wmesg, tmio );
}

void wakeup( struct socket *so, void *token ) {
    OSK_UINT flags = 0;

    OS_DbgPrint
	(OSK_MID_TRACE,("XXX Bytes to receive: %d state %x\n",
			so->so_rcv.sb_cc, so->so_state));

    if( so->so_state & SS_ISCONNECTED ) {
	OS_DbgPrint(OSK_MID_TRACE,("Socket connected!\n"));
	flags |= SEL_CONNECT;
    }
    if( so->so_q ) {
	OS_DbgPrint(OSK_MID_TRACE,("Socket accepting q\n"));
	flags |= SEL_ACCEPT;
    }
    if( so->so_rcv.sb_cc > 0 ) {
	OS_DbgPrint(OSK_MID_TRACE,("Socket readable\n"));
	flags |= SEL_READ;
    }
    if( 0 < sbspace(&so->so_snd) ) {
	OS_DbgPrint(OSK_MID_TRACE,("Socket writeable\n"));
	flags |= SEL_WRITE;
    }
    if (!so->so_pcb) {
	OS_DbgPrint(OSK_MID_TRACE,("Socket dying\n"));
	flags |= SEL_FIN;
    }

    OS_DbgPrint(OSK_MID_TRACE,("Wakeup %x (socket %x, state %x)!\n",
			       token, so,
			       so->so_state));

    if( OtcpEvent.SocketState )
	OtcpEvent.SocketState( OtcpEvent.ClientData,
			       so,
			       so ? so->so_connection : 0,
			       flags );

    if( OtcpEvent.Wakeup )
	OtcpEvent.Wakeup( OtcpEvent.ClientData, token );

    OS_DbgPrint(OSK_MID_TRACE,("Wakeup done %x\n", token));
}

/* ---------------------------------------------------------------------- */


static void
timeout_init(void)
{
	int i;

	callout = (struct callout *)
	    malloc(sizeof(struct callout) * ncallout, M_FREE, M_WAITOK);
	if (!callout)
	        panic("can't allocate callout queue!\n");

	/*
	 * Initialize callouts
	 */
	callfree = callout;
	for (i = 1; i < ncallout; i++)
		callout[i-1].c_next = &callout[i];

}

/* get clock up and running */
void clock_init()
{
	timeout_init();
	/* inittodr(0); // what does this do? */
	/* boottime = kern_time; */
	/* Start a clock we can use for timeouts */
}

