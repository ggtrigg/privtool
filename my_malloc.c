
#ifdef MALLOC_TEST
#include <malloc.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#define	CHECK_MAGIC	0xDEADBABE

typedef struct _mallref {
	char	*data;
	long	*real_data;
	int	length;
	int	real_length;
	char	file [24];
	int	line;

	struct	_mallref	*next;
	struct	_mallref	*prev;

} mall_ref;

static	mall_ref	*mall_start, *mall_end;
static	long		total_size = 0l;

char	*my_malloc (size, file, line)

int	size,line;
char	*file;

{
	mall_ref	*m,*mp;
	char		*d;
	long		*l;
	int		sz;

	total_size += size;

	/* Round up to multiple of four, and add eight for check-words */

	sz = ((size + 3) & ~3) + 8;

	l = malloc (sz);
	if (!l)
		return NULL;

	*l = CHECK_MAGIC;
	l[sz/sizeof(long) - 1] = CHECK_MAGIC;

	d = (char *)(l+1);

	m = (mall_ref *) malloc (sizeof (mall_ref));
	if (!m) {
		free (l);
		return NULL;
	}

	m->data = d;
	m->real_data = l;
	m->length = size;
	m->real_length = sz;
	strcpy (m->file,file);
	m->line = line;

	m->next = NULL;
	m->prev = NULL;

	if (!mall_start) {
		mall_start = mall_end = m;
		return d;
	}

	mp = mall_start;

	while (mp && mp->data < d) {
		mp = mp->next;
	}

	if (!mp) {
		mall_end->next = m;
		m->prev = mall_end;
		mall_end = m;
	}
	else {
		m->next = mp->next;
		m->prev = mp;
		if (mp->next) 
			mp->next->prev = m;
		else
			mall_end = m;

		mp->next = m;
	}

	return d;
}

void	my_free (d)

char	*d;

{
	mall_ref	*m;

	m  = mall_start;

	while (m && m->data != d) 
		m = m->next;

	if (m) {
		if (m->prev) 
			m->prev->next = m->next;
		else
			mall_start = m->next;

		if (m->next)
			m->next->prev = m->prev;
		else	
			mall_end = m->prev;

		d = (char *)m->real_data;
		free (m);
	}

	free (d);
}

void	malloc_dump ()

{
	mall_ref	*m;
	long		*l;

	m = mall_start;

	fprintf (stderr, "malloc_dump: %ld bytes allocated\n",total_size);
	fprintf (stderr, "Malloced areas unfreed :\n");
	if (!m)
		fprintf (stderr, "None.\n");
	else {
		while (m) {
			printf ("%d bytes from %s line %d ",
				m->length, m->file, m->line);
			l = m->real_data;

			if (*l != CHECK_MAGIC)
				printf("UNDERFLOW ! ");
			if (l[(m->real_length/sizeof(long)) - 1] != CHECK_MAGIC)
				printf("OVERFLOW !");

			putchar('\n');
			
			m = m->next;
		}
	}
}

char	*my_realloc (d, size, file, line)

char	*d, *file;
int	size, line;

{
	mall_ref	*m;
	long		*l;
	int		sz;

	m  = mall_start;

	while (m && m->data != d) 
		m = m->next;

	if (!m)
		return realloc (d, size);

	l = m->real_data;
	sz = m->real_length;

	if (*l != CHECK_MAGIC || l[sz/sizeof (long) - 1] != CHECK_MAGIC)
		fprintf(stderr, "Heap corruption detected !\n");

	sz = ((size + 3) & ~3) + 8;

	l = realloc (l, sz);

	if (l) {
		*l = CHECK_MAGIC;
		l[sz/sizeof (long) - 1] = CHECK_MAGIC;
		d = (char *)(l+1);

		total_size += (size - m->length);
		m->data = d;
		m->real_data = l;
		m->real_length = sz;
		strcpy (m->file, file);
		m->line = line;
		m->length = size;
	}
	else
		d = NULL;

	return d;
}
#endif

