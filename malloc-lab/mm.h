#include <stdio.h>

/* WSIZE: 헤더/푸터 단위 워드 크기 */
#define WSIZE       4

/* DSIZE: 두 배 워드 크기(8바이트 정렬 단위) */
#define DSIZE       8

/* CHUNKSIZE: 힙 확장 시 요청하는 기본 크기(4KB) */
#define CHUNKSIZE   (1<<12)

/* MAX: 두 값 중 큰 수 반환 */
#define MAX(x, y) ((x) > (y) ? (x) : (y))

/* PACK: 크기와 할당 비트를 하나의 워드로 패킹 */
#define PACK(size, alloc) ((size) | (alloc))

/* GET/PUT: 워드 단위 읽기·쓰기 */
#define GET(p)          (*(unsigned int *)(p))
#define PUT(p, val)     (*(unsigned int *)(p) = (val))

/* GET_SIZE: 헤더/푸터에서 블록 크기 추출 */
#define GET_SIZE(p)     (GET(p) & ~0x7)

/* GET_ALLOC: 헤더/푸터에서 할당 비트 추출 */
#define GET_ALLOC(p)    (GET(p) & 0x1)

/* HDRP/FTRP: bp로부터 헤더·푸터 주소 계산 */
#define HDRP(bp)        ((char *)(bp) - WSIZE)
#define FTRP(bp)        ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

/* NEXT_BLKP/PREV_BLKP: 인접 블록의 bp 계산 */
#define NEXT_BLKP(bp)   ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp)   ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

#define OVERHEAD (2 * WSIZE)   // 헤더(4) + 푸터(4) = 8바이트

/* 인터페이스: 힙 초기화, 할당, 해제, 재할당 */
extern int  mm_init(void);
extern void *mm_malloc(size_t size);
extern void  mm_free(void *ptr);
extern void *mm_realloc(void *ptr, size_t size);


/* 
 * Students work in teams of one or two.  Teams enter their team name, 
 * personal names and login IDs in a struct of this
 * type in their bits.c file.
 */
typedef struct {
    char *teamname; /* ID1+ID2 or ID1 */
    char *name1;    /* full name of first member */
    char *id1;      /* login ID of first member */
    char *name2;    /* full name of second member (if any) */
    char *id2;      /* login ID of second member */
} team_t;

extern team_t team;

