#include <stdio.h>
#include <gcrypt.h>

/*
 * This is an implementation of the 
 * ddsa algorithm as specified in rfc6979 
 * 
 * */
int main( int argc , char * argv[]){
	

    void* dsa_buf; 
    
    gcry_sexp_t dsa_key_pair;
    gcry_sexp_t dsa_pub_key;
    gcry_sexp_t ciphertext , plaintext, ptx2;
    
    gcry_sexp_t r_param, r_tilda_param, k_tilda_param;
    gcry_sexp_t s_param, s_tilda_param;
    gcry_sexp_t g_param;
    gcry_sexp_t p_param;
    gcry_sexp_t q_param;
    
    gcry_error_t err;
    
    gcry_mpi_t msg_digest;
    
    
    gcry_mpi_t r , r_tilda, k_tilda;
    gcry_mpi_t s , s_tilda;
    gcry_mpi_t g;
    gcry_mpi_t p;
    gcry_mpi_t q;

    int n = 1101 , i=0; // size of the file in which we have the dsa keys 
    
    
    /* Let's retreive the key from the file */
    //----------------------------------------
    
    FILE* lockf = fopen("DSA_KEY", "rb");
    
    if (!lockf) {
        puts("fopen() failed");
    }
    
    dsa_buf = malloc(n);
    if (!dsa_buf) {
        puts("malloc: could not allocate rsa buffer");
    }
    
    if (fread(dsa_buf, n, 1, lockf) != 1) {
       puts("fread() failed");
    }
    
    err = gcry_sexp_new(&dsa_key_pair, dsa_buf, n, 0);
    
    if(err){
        puts("Error");
    }


    //gcry_sexp_dump(dsa_key_pair);
    
   //----------------------------------------
    
   //digest of "Hello world." 
    const unsigned char* digest = (const unsigned char * ) "e44f3364019d18a151cab7072b5a40bb5b3e274f";
    
    err = gcry_mpi_scan(&msg_digest, GCRYMPI_FMT_USG, digest, 
                        strlen((const char*) digest), NULL);

    //*************** CORRECT SIGNATURE ********************//

    //20 is the mdlen of sha1 as specified in https://lists.gnupg.org/pipermail/gnupg-devel/2013-September/027916.html
    //a well formatted number for the immaediate has an even number of digits
    err = gcry_sexp_build(&plaintext, NULL, "(data (flags rfc6979) (hash %s %b))" , "sha1", 20 , msg_digest);
    
    err = gcry_pk_sign(&ciphertext, plaintext, dsa_key_pair);


    //now let's convert the s-expression representing r into an MPI in order
    //to use it in the equation of the attack 

    //--------- CIPHERTEXT --------------
    
    r_param = gcry_sexp_find_token(ciphertext, "r", 0);
    r = gcry_sexp_nth_mpi ( r_param , 1, GCRYMPI_FMT_USG);
         
    s_param = gcry_sexp_find_token(ciphertext, "s", 0);
    s = gcry_sexp_nth_mpi ( s_param , 1, GCRYMPI_FMT_USG);


    //--------- PUB KEY --------------
    
    g_param = gcry_sexp_find_token(dsa_key_pair, "g", 0);
    g = gcry_sexp_nth_mpi ( g_param , 1, GCRYMPI_FMT_USG);
    
    p_param = gcry_sexp_find_token(dsa_key_pair, "p", 0);
    p = gcry_sexp_nth_mpi ( p_param , 1, GCRYMPI_FMT_USG);
    
    q_param = gcry_sexp_find_token(dsa_key_pair, "q", 0);
    q = gcry_sexp_nth_mpi ( q_param , 1, GCRYMPI_FMT_USG);

    
    //*************** FAULTY SIGNATURE ********************//

    err = gcry_sexp_build(&ptx2, NULL, "(data (flags rfc6979) (hash %s %b) (attack))" , "sha1", 20 , msg_digest);

    err = gcry_pk_sign(&ciphertext, ptx2, dsa_key_pair);

    s_tilda_param = gcry_sexp_find_token(ciphertext, "s", 0);
    s_tilda = gcry_sexp_nth_mpi ( s_tilda_param , 1, GCRYMPI_FMT_USG);

    r_tilda_param = gcry_sexp_find_token(ciphertext, "r", 0);
    r_tilda = gcry_sexp_nth_mpi ( r_tilda_param , 1, GCRYMPI_FMT_USG);

    k_tilda_param = gcry_sexp_find_token(ciphertext, "k", 0);
    k_tilda = gcry_sexp_nth_mpi ( k_tilda_param , 1, GCRYMPI_FMT_USG);

    gcry_mpi_t eight = mpi_set_ui(NULL, 8);

    gcry_mpi_t result = gcry_mpi_new(mpi_get_nbits(r));

    gcry_mpi_powm(g,g,eight,p);  //g^8 mod p --------> (g^(2^3) mod p)

    gcry_mpi_mod(g,g,q);    //(g^(2^3) mod p) mod q

    gcry_mpi_mulm(result, r_tilda, g, q);   //r_tilda * g^(2^3) mod p mod q == r   -------> r_tilda = ( g^k_tilda mod p ) mod q  =  ( g^(k - 2^3) mod p ) mod q

    printf("R CALCULED\n");
    gcry_mpi_dump(result);
    printf("\n");


}

