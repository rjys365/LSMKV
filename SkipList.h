#ifndef SKIPLIST_H
#define SKIPLIST_H

#include<cmath>
#include<random>
#include<algorithm>
#include<limits>
#include<map>

#define SKIPLIST_MAX_LEVEL 32

template<class Key,class Value>
class SkipList;

template<class Key,class Value>
class SkipListNode{
    friend class SkipList<Key,Value>;
    SkipListNode **forward;
    Key key;
    Value value;
    int level;
    inline SkipListNode(Key k=Key(),Value v=Value(),int l=SKIPLIST_MAX_LEVEL,SkipListNode *nxt=nullptr)
        :key(k),value(v),level(l){
            forward=new SkipListNode *[l+1];
            for(int i=0;i<l+1;i++)forward[i]=nxt;
        }
    inline ~SkipListNode(){
        if(forward!=nullptr)delete[] forward;
    }
};

template<class Key,class Value>
class SkipList{
    private:
        //static constexpr int MAX_LEVEL=32;
        const double GROW_THRESHOLD;
        int siz=0,level=0;
        std::mt19937 rgen;
        std::uniform_real_distribution<> rdis=std::uniform_real_distribution<>(0.0,1.0);
        SkipListNode<Key,Value> *head,*tail;
        inline bool ok_to_grow(){
            return rdis(rgen)<GROW_THRESHOLD;
        }
        inline int rand_level(){
            int lv=1;
            while(ok_to_grow())lv++;
            return std::min(lv,SKIPLIST_MAX_LEVEL);//should not exceed the limit
        }

    public:
        SkipList(double threshold=0.5):GROW_THRESHOLD(threshold){
            std::random_device rdev;
            rgen.seed(rdev());
            tail=new SkipListNode<Key,Value>(std::numeric_limits<Key>::max(),Value(),0);
            head=new SkipListNode<Key,Value>(std::numeric_limits<Key>::max(),Value(),SKIPLIST_MAX_LEVEL,tail);
        }
        ~SkipList(){
            if(head!=nullptr){
                SkipListNode<Key,Value> *tmpnext,*tmpcur=head;
                while(tmpcur!=tail){
                    tmpnext=tmpcur->forward[0];
                    delete tmpcur;
                    tmpcur=tmpnext;
                }
                delete tail;
            }
        }
        void clear(){
            if(head!=nullptr){
                SkipListNode<Key,Value> *tmpnext,*tmpcur=head;
                while(tmpcur!=tail){
                    tmpnext=tmpcur->forward[0];
                    delete tmpcur;
                    tmpcur=tmpnext;
                }
                delete tail;
            }
            std::random_device rdev;
            rgen.seed(rdev());
            tail=new SkipListNode<Key,Value>(std::numeric_limits<Key>::max(),Value(),0);
            head=new SkipListNode<Key,Value>(std::numeric_limits<Key>::max(),Value(),SKIPLIST_MAX_LEVEL,tail);
            siz=0;
            level=0;
        }

        void insert(const Key &k,Value v){
            SkipListNode<Key,Value> *update[SKIPLIST_MAX_LEVEL+1];
            SkipListNode<Key,Value> *cur_ptr=head;

            for(int i=level;i>=0;i--){
                while(cur_ptr->forward[i]!=nullptr&&cur_ptr->forward[i]->key<k){
                    cur_ptr=cur_ptr->forward[i];
                }
                update[i]=cur_ptr;
            }

            if(cur_ptr->key==k){
                cur_ptr->value=v;
                return;
            } 

            int lv=rand_level();
            if(lv>level){
                lv=++level;//grow, lv=new max level
                update[lv]=head;
            }

            SkipListNode<Key,Value> *new_node=new SkipListNode<Key,Value>(k,v,lv);//the forward will be updated later
            for(int i=lv;i>=0;i--){
                cur_ptr=update[i];
                new_node->forward[i]=cur_ptr->forward[i];
                cur_ptr->forward[i]=new_node;
            }

            siz++;
        }

        Value *at(const Key &k,int *steps=nullptr) {
            int cnt=0;//bool ret=true;
            SkipListNode<Key,Value> *cur_ptr=head;
            for(int i=level;i>=0;i--){
                while(cur_ptr->forward[i]!=nullptr&&cur_ptr->forward[i]->key<k){
                    cur_ptr=cur_ptr->forward[i];
                    cnt++;
                }
            }
            //cur_ptr->key<k
            cur_ptr=cur_ptr->forward[0];
            cnt++;
            if(steps!=nullptr)*steps=cnt;
            if(cur_ptr->key==k)return &(cur_ptr->value);
            return nullptr;
        }

        inline int size(){return this->siz;}

        class iterator{
        friend class SkipList;
        private:
            SkipList *skipList;
            SkipListNode<Key,Value> *skipListNode;
            iterator(SkipList *skipList,SkipListNode<Key,Value> *skipListNode):skipList(skipList),skipListNode(skipListNode){}
        public:
            iterator operator++(){
                this->skipListNode=this->skipListNode->forward[0];
                return *this;
            }
            iterator operator++(int){
                iterator ret=*this;
                this->skipListNode=this->skipListNode->forward[0];
                return ret;
            }
            bool operator==(const iterator &right)const{
                return this->skipList==right.skipList&&this->skipListNode==right.skipListNode;
            }
            bool operator!=(const iterator &right)const{
                return !(this->operator==(right));
            }
            const Key &getKey()const{
                return this->skipListNode->key;
            }
            const Value &getValue()const{
                return this->skipListNode->value;
            }
        };

        iterator begin(){
            return iterator(this,this->head->forward[0]);
        }
        iterator end(){
            return iterator(this,this->tail);
        }
};



#endif