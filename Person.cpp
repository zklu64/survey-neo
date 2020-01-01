#include "Person.hpp"
#include "GeometryNode.hpp"
#include <ctime>
//#include <stdio.h>

Person::Person(const float posx, const float posz, const float rotated, const Material &hair, const Material &shirt,
	const Material &pants, const Material &skin, ISound *sound, ISound *panic, Pair *pairing) {
	pos = vec3(0.0f, 0.0f, 0.0f);
	this->rotated = rotated;
	this->pairing = pairing;
	node = create(hair, shirt, pants, skin);
	node->rotate('y', rotated);
	audio = sound;
	panicAudio = panic;
	if (panicAudio != NULL) panicAudio->setIsPaused(true);
	move(posx, posz);
	react = false;
	paranoid = false;
}

Person::~Person() {
}

void Person::move(const float dposx, const float dposz) {
	vec3 temp = vec3(dposx, 0.0, dposz);
	pos += temp;
	node->translate(temp);
	if (audio != NULL) audio->setPosition(vec3df(pos.x, pos.y, pos.z));
	if (panicAudio != NULL) panicAudio->setPosition(vec3df(pos.x, pos.y, pos.z));
}

SceneNode *Person::create(const Material &hair, const Material &shirt, 
        const Material &pants, const Material &skin) {
	SceneNode *torso = new SceneNode("torso");
	GeometryNode *torsoMesh = new GeometryNode("cube", "torsoMesh");
	torsoMesh->scale(vec3(0.25, 0.39, 0.15));
	torsoMesh->material = shirt;
	torso->add_child(torsoMesh);
	
	GeometryNode *neckMesh = new GeometryNode("sphere", "neck");
	neckMesh->material = skin;
	neckMesh->scale(vec3(0.07, 0.1, 0.05));
	
	SceneNode *neck = new SceneNode("neck");
	neck->translate(vec3(0.0, 0.2, 0.0));
	neck->add_child(neckMesh);
	torso->add_child(neck);
	
	SceneNode *head = new SceneNode("head");
	head->translate(vec3(0.0, 0.15, 0.0));
	GeometryNode *headMesh = new GeometryNode("cube", "head");
	headMesh->scale(vec3(0.2, 0.2, 0.2));
	headMesh->material = skin;
	
	GeometryNode *hairMesh = new GeometryNode("cube", "hair");
	hairMesh->material = hair;
	hairMesh->scale(vec3(0.22, 0.1, 0.22));
	hairMesh->translate(vec3(0.0, 0.1, 0.0));

	head->add_child(headMesh);
	head->add_child(hairMesh);
	neck->add_child(head);

	SceneNode *shoulderL = new SceneNode("shoulderL");
	shoulderL->translate(vec3(0.14, -0.025, 0.0));
	GeometryNode *shoulderLMesh = new GeometryNode("cube", "shoulderL");
	shoulderLMesh->scale(vec3(0.1, 0.45, 0.14));
	shoulderLMesh->material = shirt;
	shoulderL->add_child(shoulderLMesh);
	GeometryNode *handL = new GeometryNode("cube", "handL");
	handL->scale(vec3(0.1, 0.1, 0.1));
	handL->translate(vec3(0.0, -0.25, 0.0));
	handL->material = skin;
	shoulderL->add_child(handL);
	torso->add_child(shoulderL);

	SceneNode *shoulderR = new SceneNode("shoulderR");
	shoulderR->translate(vec3(-0.14, -0.025, 0.0));
	GeometryNode *shoulderRMesh = new GeometryNode("cube", "shoulderR");
	shoulderRMesh->scale(vec3(0.1, 0.45, 0.14));
	shoulderRMesh->material = shirt;
	shoulderR->add_child(shoulderRMesh);
	GeometryNode *handR = new GeometryNode("cube", "handR");
	handR->scale(vec3(0.1, 0.1, 0.1));
	handR->translate(vec3(0.0, -0.25, 0.0));
	handR->material = skin;
	shoulderR->add_child(handR);
	torso->add_child(shoulderR);

	SceneNode *crotch = new SceneNode("crotch");
	crotch->translate(vec3(0.0, -0.29, 0.0));
	GeometryNode *crotchMesh = new GeometryNode("cube", "crotch");
	crotchMesh->scale(vec3(0.25, 0.2, 0.16));
	crotchMesh->material = pants;
	crotch->add_child(crotchMesh);
	GeometryNode *legL = new GeometryNode("cube", "legL");
	legL->scale(vec3(0.1, 0.4, 0.16));
	legL->translate(vec3(0.075, -0.24, 0.0));
	legL->material = pants;
	crotch->add_child(legL);

        GeometryNode *legR = new GeometryNode("cube", "legR");
        legR->scale(vec3(0.1, 0.4, 0.16));
        legR->translate(vec3(-0.075, -0.24, 0.0));
        legR->material = pants;
	crotch->add_child(legR);
	torso->add_child(crotch);
	torso->translate(vec3(0.0, 0.73, 0.0));
	return torso;
}

void Person::rotate(const float rot) {
	node->rotate('y', -rotated);
	node->rotate('y', rot);
	rotated = rot;
}

void Person::trigger() {
	if (react || (pairing != NULL && pairing->follower == this)) return; //no need to keep track of time when person AI already freaking out, plus now we can use startTime to track freaking out
	if (paranoid) {
		secondsPassed = (clock() - startTime) / CLOCKS_PER_SEC;
	} else {
		paranoid = true;
		startTime = clock();
	}
	if (secondsPassed > 20) {
		startTime = clock();
		if (audio != NULL) audio->setIsPaused(true);
		if (panicAudio != NULL) panicAudio->setIsPaused(false);
		react = true;
	}
}

void Person::run(glm::vec3 light) {
	//a person who is assigned follower role will never have react to be set to true from trigger
	if (react) {
		secondsPassed = (clock() - startTime) / CLOCKS_PER_SEC;
		//vector representing opposite direction of light to person
		glm::vec3 dir = normalize(pos-light);
		if (pairing != NULL && (pairing->leader == this)) {
			pairing->follower->move(0.2f*dir.x, 0.2f*dir.z);
		}
		move(0.2f*dir.x, 0.2f*dir.z);
		if (secondsPassed > 20) {
			if (audio != NULL) audio->setIsPaused(false);
			if (panicAudio != NULL) panicAudio->setIsPaused(true);
			secondsPassed = 0;
			paranoid = false;
			react = false;
		}
	}
}
