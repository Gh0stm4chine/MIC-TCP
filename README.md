# MIC-TCP

L’objectif du BE est de	 concevoir et	de développer	en langage C un	protocole	de niveau	Transport	appelé	MIC-TCP visant	à	transport	un	 flux	vidéo	distribué	en	temps	réel	par	une	application	(fournie	par	les	enseignants)	appelée	tsock-vidéo.

L’application	 tsock-vidéo est basée sur les spécifications suivantes : 
  ● elle génère un	flux	vidéo	et	non	du	texte	;
  ● elle permet d’invoquer,	soit TCP, soit MIC-TCP comme protocole sous jacent au	transport de la vidéo, ceci par le biais        d’une API similaire à l’API socket incluant	comme primitives principales socket(),bind(),connect(),accept(),send(),recv().
  
Ces	 primitives,	 dont	 les signatures seront	 fournies,	 devront	 être	 développées dans	 le	
cadre	du	BE,	faute	de	quoi	l’application	tsock-vidéo	ne	pourra	pas	s’exécuter.

Le protocole MIC-TCP devra inclure les phases	et mécanismes	suivants :
  ● une	phase	d’établissement	de	connexion suivant	le modèle d’établissement d’une connexion TCP étudié dans	le	cours	           d’introduction	aux	réseaux	du	semestre	5	;
  ● une	 phase de	transfert de données unidirectionnelle incluant	un mécanisme de	reprise	des	pertes de	type « Stop	&	Wait » 
  ● Le mécanisme	de	reprise	des	pertes	de	type	« Stop	&	Wait » devra	 être	 étendu	 de	 sorte	 à	 inclure	 un	 concept nouveau décrit	ci-après	 :
   
Celui	de gestion	d’une fiabilité non plus totale mais partielle. Cette extension conduira à démontrer qu’un transfert de la vidéo	via	une	version de MIC-TCP « à fiabilité partielle » conduit à une meilleure qualité de la présentation vidéo coté récepteur lorsque le	réseau génère des pertes.

Par	soucis	de	simplification	du	travail :

  ● MIC-TCP	n’inclura pas	de	mécanisme	de	gestion	du	multiplexage	/	démultiplexage	;
  ● MTCP	n’inclura pas	de	mécanisme	de	contrôle	de	flux	;
  ● MTCP	n’inclura pas	de	phase	de	fermeture	de	connexion	;
  ● MIC-TCP	assurera	un	transfert	de	données	en	mode	message	(et	non	flux	d’octets).
  
Le	 concept	 de	 gestion	 d’une	 fiabilité	 partielle est	 basiquement	 simple	 à	 comprendre.	
Appliqué	 au	 BE,	 il	 consiste	 à	 ce	 que	MIC-TCP	 soit	 autorisé	 à	 ne	 pas	 délivrer	 en	 réception	
toutes	 les	 données	 applicatives	 (ici	 des	 images)	 qui	 lui	 sont	 soumises	 en	 émission,	 à	
condition	de	respecter	un	%	maximal	de	pertes	admissibles	(par	exemple	:	20%	des	images	
peuvent	 être	 perdues).	 Le	 but	 est	 de	 permettre	 la	 délivrance	 au	 plus	 tôt	 des	 images	
contenues	dans	des	PDUs	MIC-TCP	 reçus	hors	 séquence	 (c’est	à	dire pour	lesquels	le	PDU	
attendu	a	été	perdu	suite	à	une	congestion	du	réseau),	tout	en	respectant	le	%	maximal	de	
pertes	admissibles	pour	la	bonne	exécution de	l’application.
