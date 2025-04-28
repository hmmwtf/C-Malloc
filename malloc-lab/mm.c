/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 *
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

static void *coalesce(void *);
static void *find_fit(size_t);
static void place(void *, size_t);
static void remove_node(void *);

char *heap_listp;
static void *find_nextp; //전역 포인터: Next-Fit 탐색 시작 위치 저장

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "ateam",
    /* First member's full name */
    "Harry Bovik",
    /* First member's email address */
    "bovik@cs.cmu.edu",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~0x7)

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

/*
 * extend_heap - 힙을 주어진 단어(words)만큼 확장하고,
 *               새로 확보된 블록을 자유 상태로 초기화한 뒤
 *               인접 블록과 병합하여 반환
 */
static void *extend_heap(size_t words)
{
    char *bp;
    size_t size;

    /* 1) 요청 크기 계산 (WSIZE 단위 → 바이트), 짝수 words로 맞춰 8바이트 정렬 보장 */
    size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;

    /* 2) 힙 확장 실패 시 NULL 반환 */
    bp = mem_sbrk(size);
    if (bp == (void *)-1){
        return NULL;
    }

    /* 3) 새 블록 헤더/푸터 설정 (자유 블록 표시) */
    PUT(HDRP(bp), PACK(size, 0));   /* 헤더 */
    PUT(FTRP(bp), PACK(size, 0));   /* 푸터 */

    /* 4) 새 에필로그 헤더 설정 (크기 0, 할당 표시) */
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));

    /* 5) 인접 자유 블록과 병합 후 블록 포인터 반환 */
    return coalesce(bp);
}


/*
 * mm_init - initialize the malloc package.
 */
/* 메모리 관리자 초기화 */
int mm_init(void)
{   
    // 초기 힙 공간 할당 (4워드: 정렬 패딩 + 프롤로그 + 에필로그)
    if ((heap_listp = mem_sbrk(4*WSIZE)) == (void *)-1)     // 초기 힙 메모리를 할당
        return -1;
     // 힙 구조 초기화
    PUT(heap_listp, 0);                             // 힙의 시작 부분에 0을 저장하여 패딩으로 사용
    PUT(heap_listp + (1*WSIZE), PACK(DSIZE, 1));    // 프롤로그 블럭의 헤더에 할당된 상태로 표시하기 위해 사이즈와 할당 비트를 설정하여 값을 저장
    PUT(heap_listp + (2*WSIZE), PACK(DSIZE, 1));    // 프롤로그 블록의 풋터에도 마찬가지로 사이즈와 할당 비트를 설정하여 값을 저장
    PUT(heap_listp + (3*WSIZE), PACK(0, 1));        // 에필로그 블록의 헤더를 설정하여 힙의 끝을 나타내는 데 사용
    heap_listp += (2*WSIZE);                        // 프롤로그 블록 다음의 첫 번째 바이트를 가리키도록 포인터 조정
    find_nextp = heap_listp;                      // nextfit을 위한 변수(nextfit 할 때 사용)

    // 초기 힙 확장
    if (extend_heap(CHUNKSIZE/WSIZE) == NULL)       // 초기 힙을 확장하여 충분한 양의 메모리가 사용 가능하도록 chunksize를 단어 단위로 변환하여 힙 확장
        return -1;
    if (extend_heap(4) == NULL)                     //자주 사용되는 작은 블럭이 잘 처리되어 점수가 오름
        return -1;
    return 0;
}

/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */

 // 내부 단편화만 진행한 malloc
 /*
void *mm_malloc(size_t size)
{
    int newsize = ALIGN(size + SIZE_T_SIZE);
    void *p = mem_sbrk(newsize);
    if (p == (void *)-1)
        return NULL;
    else
    {
        *(size_t *)p = size;
        return (void *)((char *)p + SIZE_T_SIZE);
    }
}
*/

void *mm_malloc(size_t size)
{
    size_t asize;
    size_t extendsize;
    char *bp;

    if (size == 0) {
        return NULL;
    }

    if (size <= DSIZE) {
        asize = 2 * DSIZE;
    } else {
        asize = DSIZE * ((size + (DSIZE) + (DSIZE - 1)) / DSIZE);
    }

    if ((bp = find_fit(asize)) != NULL) {
        place(bp, asize);
        return bp;
    }

    extendsize = MAX(asize, CHUNKSIZE);
    if ((bp = extend_heap(extendsize/WSIZE)) == NULL) {
        return NULL;
    }
    place(bp, asize);
    return bp;
}

static void *coalesce(void *bp) {
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));  /* 이전 블록 푸터에서 할당 비트 */
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));  /* 다음 블록 헤더에서 할당 비트 */
    size_t size       = GET_SIZE(HDRP(bp));             /* 현재 블록 크기 */

    /* case 1: 앞뒤 모두 할당 → 병합 없이 그대로 반환 */
    if (prev_alloc && next_alloc) {
        return bp;
    }
    /* case 2: 앞은 할당, 뒤만 자유 → 다음 블록과 병합 */
    else if (prev_alloc && !next_alloc) {
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));          /* 다음 블록 크기 추가 */
        PUT(HDRP(bp), PACK(size, 0));                   /* 새 헤더 */
        PUT(FTRP(bp), PACK(size, 0));                   /* 새 푸터 */
    }
    /* case 3: 앞만 자유, 뒤는 할당 → 이전 블록과 병합 */
    else if (!prev_alloc && next_alloc) {
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));          /* 이전 블록 크기 추가 */
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));        /* 이전 블록 헤더 갱신 */
        PUT(FTRP(bp), PACK(size, 0));
        bp = PREV_BLKP(bp);                             /* 반환할 bp는 이전 블록 */
    }
    /* case 4: 앞뒤 모두 자유 → 양쪽 모두와 병합 */
    else {
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(HDRP(NEXT_BLKP(bp))); 
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));        /* 새 헤더 (이전 블록 위치) */
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));        /* 새 푸터 (다음 블록 끝) */
        bp = PREV_BLKP(bp);                             /* 반환할 bp는 병합된 블록의 시작 */
    }
    find_nextp = bp;
    return bp;  /* 병합된 블록의 payload 포인터 반환 */ 
}

/* 구현 목표: First - fit, Next - fit, Best - Fit*/
// First - Fit
/*
static void *find_fit(size_t asize) {
    void *bp;

    // 1) 힙 시작부터 블록 크기 > 0 (에필로그 전)까지 순회
    for (bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)) {
        // 2) 자유 블록인지 & 충분한 크기인지 검사 
        if (!GET_ALLOC(HDRP(bp)) && GET_SIZE(HDRP(bp)) >= asize) {
            return bp;  // 첫 번째 적합 블록 반환
        }
    }

    // 3) 적합 블록 없음 
    return NULL;
}
*/

// Next - fit
/* 적합한 블록 탐색 (Next-fit) */

static void *find_fit(size_t asize) {
    void *ptr;
    ptr = find_nextp;   // 현재 탐색 시작 위치를 저장

    // 첫 번째 루프: 현재 find_nextp부터 힙 끝까지 탐색
    for (; GET_SIZE(HDRP(find_nextp)) >0; find_nextp = NEXT_BLKP(find_nextp))
    {   
        // 만약 블록이 할당되어 있지 않고, 크기가 요청한 크기 이상이면
        if (!GET_ALLOC(HDRP(find_nextp)) && (asize <= GET_SIZE(HDRP(find_nextp))))
        {
            return find_nextp;  // 해당 블록을 반환
        }
    }
    // 두 번째 루프: 힙 처음(heap_listp)부터 아까 저장해둔 ptr까지 탐색
    for (find_nextp = heap_listp; find_nextp != ptr; find_nextp = NEXT_BLKP(find_nextp))
    {   
        // 마찬가지로 할당되어 있지 않고 크기가 충분하면
        if (!GET_ALLOC(HDRP(find_nextp)) && (asize <= GET_SIZE(HDRP(find_nextp))))
        {
            return find_nextp;  // 해당 블록을 반환
        }
    }
    return NULL;    // 적합한 블록을 찾지 못한 경우
}


// Best - fit
/*
static void *find_fit(size_t asize) {
    void *bp = heap_listp;
    void *best_bp = NULL;
    size_t best_size = (size_t)-1;        // SIZE_MAX 

    for (; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)) {
        if (!GET_ALLOC(HDRP(bp))) {
            size_t blk_size = GET_SIZE(HDRP(bp));
            if (blk_size >= asize && blk_size < best_size) {
                best_size = blk_size;
                best_bp   = bp;
                if (best_size == asize)
                    break;                //Perfect fit
            }
        }
    }

    return best_bp;                       // NULL 가능
}
*/

static void place(void *bp, size_t asize) {
    size_t csize = GET_SIZE(HDRP(bp));

    if ((csize - asize) >= (2 * DSIZE)) {
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));
        bp = NEXT_BLKP(bp);
        PUT(HDRP(bp), PACK(csize - asize, 0));
        PUT(FTRP(bp), PACK(csize - asize, 0));
    } else {
        PUT(HDRP(bp), PACK(csize, 1));
        PUT(FTRP(bp), PACK(csize, 1));
    }
}

/*
 * mm_free - Freeing a block does nothing.
 */
/*
 * mm_free - 주어진 블록(bp)을 “자유” 상태로 표시한 뒤,
 *           인접한 자유 블록과 병합(coalesce) 처리
 */
void mm_free(void *bp)
{
    size_t size = GET_SIZE(HDRP(bp));  /* 1) 현재 블록 전체 크기 확보 */

    /* 2) 헤더·푸터에 ‘할당 비트 = 0’으로 기록하여 블록을 자유로 표시 */
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));

    /* 3) 병합 처리: 앞뒤 블록 중 자유 블록이 있으면 크기를 합침 */
    coalesce(bp);  /* 항상 coalesce 호출 (경계 태그 기법) */  
}

/*
 * coalesce - 인접 블록들과 병합하여 단편화 감소
 *             prev_alloc: 이전 블록 할당 여부
 *             next_alloc: 다음 블록 할당 여부
 *             size:        병합 대상 블록의 크기
 */


/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */

 /*
void *mm_realloc(void *ptr, size_t size)
{
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;

    newptr = mm_malloc(size);
    if (newptr == NULL)
        return NULL;
    copySize = *(size_t *)((char *)oldptr - SIZE_T_SIZE);
    if (size < copySize)
        copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
}
*/

void *mm_realloc(void *ptr, size_t size)
{
    if (ptr == NULL) {
        // realloc(NULL, size)는 malloc(size)와 같음
        return mm_malloc(size);
    }

    if (size == 0) {
        // realloc(ptr, 0)은 free(ptr) 후 NULL 반환
        mm_free(ptr);
        return NULL;
    }

    size_t oldsize = GET_SIZE(HDRP(ptr));
    size_t asize;

    if (size <= DSIZE) {
        asize = 2 * DSIZE;
    } else {
        asize = DSIZE * ((size + (DSIZE) + (DSIZE - 1)) / DSIZE);
    }

    // [1] 새 요청 크기가 기존 블록보다 작거나 같으면 그냥 반환
    if (asize <= oldsize) {
        return ptr;
    }

    // [2] 다음 블록이 free고 합쳐서 충분하면 합치기
    void *next_bp = NEXT_BLKP(ptr);
    if (!GET_ALLOC(HDRP(next_bp))) {
        size_t nextsize = GET_SIZE(HDRP(next_bp));
        if (oldsize + nextsize >= asize) {
            remove_node(ptr);
            // 다음 블록과 합쳐서 크기 키우기
            size_t newsize = oldsize + nextsize;
            PUT(HDRP(ptr), PACK(newsize, 1));
            PUT(FTRP(ptr), PACK(newsize, 1));
            return ptr;
        }
    }

    // [3] 둘 다 안 되면 새로 malloc → 데이터 복사 → old free
    void *newptr = mm_malloc(size);
    if (newptr == NULL) {
        return NULL;
    }

    size_t copySize = oldsize - DSIZE; // 실제 payload 크기
    if (size < copySize) {
        copySize = size;
    }
    memcpy(newptr, ptr, copySize);
    mm_free(ptr);
    return newptr;
}

static void remove_node(void *bp)
{
    // 다음 블록 free, 할당 여부
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    // 현재 블록 크기
    size_t size = GET_SIZE(HDRP(bp));             

    // Case2: 이전 할당, 다음 free인 경우
    if (!next_alloc) {
        // 다음 블록 크기 추가
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));         

        // 병합된 내용을 헤더와, 풋터에 추가
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    }
    
    //  병합 끝난 뒤에, 업데이트
    find_nextp = bp;    
}